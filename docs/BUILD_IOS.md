# Build QK4 Mobile for iOS

The iOS target must be configured and built on macOS with Xcode and a Qt iOS kit. The Windows and Android scripts do not produce an iOS application.

## Requirements

- Xcode with the iOS 16 or newer SDK
- Qt 6.11.1 for iOS, matching the Qt version used by the project
- CMake through Qt's `qt-cmake` wrapper
- The checked-in iOS OpenSSL and Opus headers and static libraries under `third_party/ios`

The K4 remote connection uses TLS 1.2 PSK. Qt's iOS TLS backend does not support PSK, so this target links OpenSSL and uses `PskTlsSocket` directly. QRZ API keys are stored in the iOS Keychain with the `AfterFirstUnlockThisDeviceOnly` accessibility class.

## Configure and build

From the repository root on the Mac:

```bash
/path/to/Qt/6.11.1/ios/bin/qt-cmake -S . -B build-ios -G Xcode
cmake --build build-ios --config RelWithDebInfo -- -sdk iphoneos
```

For an Intel Simulator build:

```bash
cmake --build build-ios --config RelWithDebInfo -- \
  -sdk iphonesimulator -arch x86_64 CODE_SIGNING_ALLOWED=NO ONLY_ACTIVE_ARCH=YES
```

The bundled archives contain device `arm64` and Intel Simulator `x86_64` slices. An Apple Silicon Simulator needs separately built simulator-arm64 dependencies packaged as XCFrameworks.

Set the Apple development team and signing identity in the generated Xcode project before installing on a physical device. The bundle identifier is `com.w9wdx.qk4phone`.

## Screen behavior

The radio console opens in landscape. FT8 and FT4 request portrait. SSTV and a logbook opened from the radio or SSTV allow portrait and landscape, then return to the landscape radio console when closed.

## Validation boundary

Windows tests and an Android build verify shared C++ and the Qt TLS adapter. The iOS target, Keychain, orientation requests, direct OpenSSL PSK handshake, audio routes, and physical-device UI require validation on a Mac and iPhone/iPad before release.
