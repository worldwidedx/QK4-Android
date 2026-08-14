package com.ai5qk.qk4phone;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.util.Base64;
import android.util.Log;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;

/** Android BLE/USB MIDI discovery and input bridge for QK4's CW keyer path. */
public final class AndroidBleMidi {
    private static final String TAG = "QK4-Midi";
    private static final UUID MIDI_SERVICE = UUID.fromString("03b80e5a-ede8-4b33-a751-6ce34ec4c700");
    private static final int PERMISSION_REQUEST = 7406;
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    private static final Map<String, BluetoothDevice> BLE_DEVICES = new LinkedHashMap<>();
    private static final Map<String, MidiDeviceInfo> USB_DEVICES = new LinkedHashMap<>();
    private static final ConcurrentLinkedQueue<Integer> EVENTS = new ConcurrentLinkedQueue<>();

    private static BluetoothLeScanner scanner;
    private static ScanCallback scanCallback;
    private static MidiDevice midiDevice;
    private static MidiOutputPort outputPort;
    private static MidiReceiver midiReceiver;
    private static volatile int connectionState; // 0 disconnected, 1 connecting, 2 connected, 3 error
    private static volatile String statusMessage = "Not connected";

    private AndroidBleMidi() {}

    public static void startScan(Context context) {
        stopScan();
        synchronized (BLE_DEVICES) { BLE_DEVICES.clear(); }
        final int usbCount = refreshUsbDevices(context);

        if (!ensurePermissions(context)) {
            statusMessage = usbCount > 0
                    ? "Found " + usbCount + " USB MIDI device(s); allow Bluetooth to scan BLE MIDI"
                    : "Bluetooth permission requested; tap Scan again after allowing it";
            return;
        }
        final BluetoothManager manager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        if (manager == null || manager.getAdapter() == null || !manager.getAdapter().isEnabled()) {
            statusMessage = usbCount > 0
                    ? "Found " + usbCount + " USB MIDI device(s); Bluetooth is off"
                    : "No USB MIDI devices found; Bluetooth is off";
            return;
        }
        scanner = manager.getAdapter().getBluetoothLeScanner();
        if (scanner == null) {
            statusMessage = usbCount > 0
                    ? "Found " + usbCount + " USB MIDI device(s); BLE scanner unavailable"
                    : "BLE scanner unavailable";
            return;
        }
        scanCallback = new ScanCallback() {
            @Override public void onScanResult(int callbackType, ScanResult result) {
                final BluetoothDevice device = result.getDevice();
                if (device == null) return;
                final String address = device.getAddress();
                final boolean isNew;
                synchronized (BLE_DEVICES) { isNew = BLE_DEVICES.put(address, device) == null; }
                if (isNew) Log.i(TAG, "Found BLE MIDI device " + safeName(device) + " " + address);
            }

            @Override public void onScanFailed(int errorCode) {
                statusMessage = "BLE scan failed (" + errorCode + ")";
                Log.w(TAG, statusMessage);
            }
        };
        final List<ScanFilter> filters = new ArrayList<>();
        filters.add(new ScanFilter.Builder().setServiceUuid(new ParcelUuid(MIDI_SERVICE)).build());
        scanner.startScan(filters,
                new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), scanCallback);
        statusMessage = usbCount > 0
                ? "Found " + usbCount + " USB MIDI device(s); scanning BLE MIDI..."
                : "Scanning USB and BLE MIDI devices...";
        MAIN.postDelayed(AndroidBleMidi::stopScan, 8000);
    }

    public static String getDevices() {
        final StringBuilder result = new StringBuilder();
        synchronized (USB_DEVICES) {
            for (Map.Entry<String, MidiDeviceInfo> entry : USB_DEVICES.entrySet()) {
                if (result.length() > 0) result.append('\n');
                result.append(deviceName(entry.getValue())).append(" (USB)|").append(entry.getKey());
            }
        }
        synchronized (BLE_DEVICES) {
            for (Map.Entry<String, BluetoothDevice> entry : BLE_DEVICES.entrySet()) {
                if (result.length() > 0) result.append('\n');
                result.append(safeName(entry.getValue())).append(" (BLE)|ble:").append(entry.getKey());
            }
        }
        return result.toString();
    }

    public static boolean connect(Context context, String deviceKey) {
        if (deviceKey != null && deviceKey.startsWith("usb:")) return connectUsb(context, deviceKey);
        if (!ensurePermissions(context)) return false;

        final String address = deviceKey != null && deviceKey.startsWith("ble:")
                ? deviceKey.substring(4) : deviceKey; // Raw address supports saved pre-USB builds.
        BluetoothDevice foundDevice;
        synchronized (BLE_DEVICES) { foundDevice = BLE_DEVICES.get(address); }
        if (foundDevice == null) {
            final BluetoothManager bluetoothManager =
                    (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
            try {
                if (bluetoothManager != null && bluetoothManager.getAdapter() != null)
                    foundDevice = bluetoothManager.getAdapter().getRemoteDevice(address);
            } catch (IllegalArgumentException | SecurityException ignored) {}
        }
        final BluetoothDevice device = foundDevice;
        if (device == null) {
            connectionState = 3;
            statusMessage = "Saved BLE MIDI device is unavailable; scan again";
            return false;
        }
        disconnect();
        stopScan();
        final MidiManager manager = (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
        if (manager == null) {
            connectionState = 3;
            statusMessage = "Android MIDI service unavailable";
            return false;
        }
        connectionState = 1;
        statusMessage = "Connecting to " + safeName(device) + " over BLE...";
        manager.openBluetoothDevice(device, opened -> finishOpen(opened, safeName(device), "BLE"), MAIN);
        return true;
    }

    private static boolean connectUsb(Context context, String deviceKey) {
        final MidiManager manager = (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
        if (manager == null) {
            connectionState = 3;
            statusMessage = "Android MIDI service unavailable";
            return false;
        }
        refreshUsbDevices(context);
        final MidiDeviceInfo info;
        synchronized (USB_DEVICES) { info = USB_DEVICES.get(deviceKey); }
        if (info == null) {
            connectionState = 3;
            statusMessage = "Saved USB MIDI device is not attached; scan again";
            return false;
        }
        disconnect();
        stopScan();
        final String name = deviceName(info);
        connectionState = 1;
        statusMessage = "Connecting to " + name + " over USB...";
        manager.openDevice(info, opened -> finishOpen(opened, name, "USB"), MAIN);
        return true;
    }

    private static void finishOpen(MidiDevice opened, String name, String transport) {
        if (opened == null) {
            connectionState = 3;
            statusMessage = "Could not open " + name + " over " + transport;
            return;
        }
        midiDevice = opened;
        for (MidiDeviceInfo.PortInfo port : opened.getInfo().getPorts()) {
            if (port.getType() != MidiDeviceInfo.PortInfo.TYPE_OUTPUT) continue;
            outputPort = opened.openOutputPort(port.getPortNumber());
            if (outputPort == null) continue;
            midiReceiver = new MidiReceiver() {
                @Override public void onSend(byte[] data, int offset, int count, long timestamp) {
                    parseMidi(data, offset, count);
                }
            };
            outputPort.connect(midiReceiver);
            connectionState = 2;
            statusMessage = "Connected to " + name + " (" + transport + ")";
            Log.i(TAG, statusMessage);
            return;
        }
        connectionState = 3;
        statusMessage = name + " has no MIDI output port";
        closeDevice();
    }

    public static void disconnect() {
        connectionState = 0;
        statusMessage = "Not connected";
        EVENTS.clear();
        closeDevice();
    }

    public static int getConnectionState() { return connectionState; }
    public static String getStatusMessage() { return statusMessage; }
    public static int pollEvent() {
        final Integer event = EVENTS.poll();
        return event == null ? -1 : event;
    }

    private static void parseMidi(byte[] data, int offset, int count) {
        for (int i = offset; i + 2 < offset + count; ) {
            final int status = data[i] & 0xff;
            final int kind = status & 0xf0;
            if (kind == 0x80 || kind == 0x90 || kind == 0xb0) {
                final int data1 = data[i + 1] & 0x7f;
                final int data2 = data[i + 2] & 0x7f;
                EVENTS.offer((status << 16) | (data1 << 8) | data2);
                Log.d(TAG, "MIDI status=" + status + " data1=" + data1 + " data2=" + data2);
                i += 3;
            } else {
                i++;
            }
        }
    }

    private static void stopScan() {
        if (scanner != null && scanCallback != null) {
            try { scanner.stopScan(scanCallback); } catch (SecurityException ignored) {}
        }
        scanCallback = null;
        scanner = null;
    }

    private static void closeDevice() {
        if (outputPort != null) {
            try { outputPort.close(); } catch (IOException ignored) {}
            outputPort = null;
        }
        midiReceiver = null;
        if (midiDevice != null) {
            try { midiDevice.close(); } catch (IOException ignored) {}
            midiDevice = null;
        }
    }

    @SuppressWarnings("deprecation")
    private static int refreshUsbDevices(Context context) {
        final MidiManager manager = (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
        synchronized (USB_DEVICES) {
            USB_DEVICES.clear();
            if (manager == null) return 0;
            for (MidiDeviceInfo info : manager.getDevices()) {
                if (info.getType() != MidiDeviceInfo.TYPE_USB || info.getOutputPortCount() < 1) continue;
                final String key = usbDeviceKey(info);
                USB_DEVICES.put(key, info);
                Log.i(TAG, "Found USB MIDI device " + deviceName(info));
            }
            return USB_DEVICES.size();
        }
    }

    private static String usbDeviceKey(MidiDeviceInfo info) {
        final Bundle properties = info.getProperties();
        final UsbDevice usb = properties.getParcelable(MidiDeviceInfo.PROPERTY_USB_DEVICE);
        final String identity = (usb == null ? "0:0" : usb.getVendorId() + ":" + usb.getProductId())
                + ":" + propertyString(properties, MidiDeviceInfo.PROPERTY_MANUFACTURER)
                + ":" + propertyString(properties, MidiDeviceInfo.PROPERTY_PRODUCT)
                + ":" + propertyString(properties, MidiDeviceInfo.PROPERTY_SERIAL_NUMBER);
        return "usb:" + Base64.encodeToString(identity.getBytes(StandardCharsets.UTF_8),
                Base64.URL_SAFE | Base64.NO_WRAP);
    }

    private static String deviceName(MidiDeviceInfo info) {
        final Bundle properties = info.getProperties();
        String name = propertyString(properties, MidiDeviceInfo.PROPERTY_NAME);
        if (name.isEmpty()) name = propertyString(properties, MidiDeviceInfo.PROPERTY_PRODUCT);
        return name.isEmpty() ? "USB MIDI" : name;
    }

    private static String propertyString(Bundle properties, String key) {
        final String value = properties.getString(key);
        return value == null ? "" : value;
    }

    private static boolean ensurePermissions(Context context) {
        final String[] permissions;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions = new String[] { Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT };
        } else {
            permissions = new String[] { Manifest.permission.ACCESS_FINE_LOCATION };
        }
        final List<String> missing = new ArrayList<>();
        for (String permission : permissions) {
            if (context.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) missing.add(permission);
        }
        if (missing.isEmpty()) return true;
        if (context instanceof Activity) {
            final Activity activity = (Activity) context;
            activity.runOnUiThread(() -> activity.requestPermissions(
                    missing.toArray(new String[0]), PERMISSION_REQUEST));
        }
        return false;
    }

    private static String safeName(BluetoothDevice device) {
        try {
            final String name = device.getName();
            return name == null || name.isEmpty() ? "BLE MIDI" : name;
        } catch (SecurityException ignored) {
            return "BLE MIDI";
        }
    }
}
