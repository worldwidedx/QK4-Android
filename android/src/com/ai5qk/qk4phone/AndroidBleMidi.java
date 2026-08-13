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
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiDeviceStatus;
import android.media.midi.MidiManager;
import android.media.midi.MidiOutputPort;
import android.media.midi.MidiReceiver;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedQueue;

/** Android BLE-MIDI discovery and input bridge for QK4's HaliKey path. */
public final class AndroidBleMidi {
    private static final String TAG = "QK4-BleMidi";
    private static final UUID MIDI_SERVICE = UUID.fromString("03b80e5a-ede8-4b33-a751-6ce34ec4c700");
    private static final int PERMISSION_REQUEST = 7406;
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    private static final Map<String, BluetoothDevice> DEVICES = new LinkedHashMap<>();
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
        if (!ensurePermissions(context)) {
            statusMessage = "Bluetooth permission requested; tap Scan again after allowing it";
            return;
        }
        final BluetoothManager manager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        if (manager == null || manager.getAdapter() == null || !manager.getAdapter().isEnabled()) {
            statusMessage = "Bluetooth is off";
            return;
        }
        stopScan();
        synchronized (DEVICES) { DEVICES.clear(); }
        scanner = manager.getAdapter().getBluetoothLeScanner();
        if (scanner == null) {
            statusMessage = "BLE scanner unavailable";
            return;
        }
        scanCallback = new ScanCallback() {
            @Override public void onScanResult(int callbackType, ScanResult result) {
                final BluetoothDevice device = result.getDevice();
                if (device == null) return;
                final String address = device.getAddress();
                final boolean isNew;
                synchronized (DEVICES) { isNew = DEVICES.put(address, device) == null; }
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
        statusMessage = "Scanning for BLE MIDI devices…";
        MAIN.postDelayed(AndroidBleMidi::stopScan, 8000);
    }

    public static String getDevices() {
        final StringBuilder result = new StringBuilder();
        synchronized (DEVICES) {
            for (Map.Entry<String, BluetoothDevice> entry : DEVICES.entrySet()) {
                if (result.length() > 0) result.append('\n');
                result.append(safeName(entry.getValue())).append('|').append(entry.getKey());
            }
        }
        return result.toString();
    }

    public static boolean connect(Context context, String address) {
        if (!ensurePermissions(context)) return false;
        BluetoothDevice foundDevice;
        synchronized (DEVICES) { foundDevice = DEVICES.get(address); }
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
            statusMessage = "Saved device is unavailable; scan again";
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
        statusMessage = "Connecting to " + safeName(device) + "…";
        manager.openBluetoothDevice(device, opened -> {
            if (opened == null) {
                connectionState = 3;
                statusMessage = "Could not open " + safeName(device);
                return;
            }
            midiDevice = opened;
            final MidiDeviceInfo.PortInfo[] ports = opened.getInfo().getPorts();
            for (MidiDeviceInfo.PortInfo port : ports) {
                if (port.getType() != MidiDeviceInfo.PortInfo.TYPE_OUTPUT) continue;
                outputPort = opened.openOutputPort(port.getPortNumber());
                if (outputPort != null) {
                    midiReceiver = new MidiReceiver() {
                        @Override public void onSend(byte[] data, int offset, int count, long timestamp) {
                            parseMidi(data, offset, count);
                        }
                    };
                    outputPort.connect(midiReceiver);
                    connectionState = 2;
                    statusMessage = "Connected to " + safeName(device);
                    Log.i(TAG, statusMessage);
                    return;
                }
            }
            connectionState = 3;
            statusMessage = "Connected device has no MIDI output port";
            closeDevice();
        }, MAIN);
        return true;
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
            if (kind == 0x80 || kind == 0x90) {
                final int note = data[i + 1] & 0x7f;
                final int velocity = data[i + 2] & 0x7f;
                EVENTS.offer((status << 16) | (note << 8) | velocity);
                Log.d(TAG, "MIDI status=" + status + " note=" + note + " velocity=" + velocity);
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
