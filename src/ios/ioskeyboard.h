#ifndef IOSKEYBOARD_H
#define IOSKEYBOARD_H

// Begin tracking hardware-keyboard connect/disconnect events. Call once at
// startup, before the user can toggle a keyboard, so the cached state stays
// accurate. GCKeyboard only keeps its state current while the app observes its
// notifications, so a bare synchronous read is unreliable on its own.
void iosStartKeyboardMonitoring();

// Returns true when a hardware keyboard (iPad Smart/Magic Keyboard, or a USB or
// Bluetooth keyboard) is currently attached and usable. Backed by GameController
// (GCKeyboard) on iOS 14+. Used so FREQ ENT can offer the inline edit field when
// a keyboard is present and the on-screen numeric keypad when it is not.
bool iosHasHardwareKeyboard();

#endif // IOSKEYBOARD_H
