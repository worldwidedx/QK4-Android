#import "ioskeyboard.h"

#import <GameController/GameController.h>

// Cached hardware-keyboard presence. GCKeyboard.coalescedKeyboard only tracks
// disconnects reliably once the app has registered for its notifications, so a
// bare synchronous read can stay non-nil after a Bluetooth keyboard is turned
// off. Register connect/disconnect observers at startup and cache the state.
static bool s_hasHardwareKeyboard = false;

void iosStartKeyboardMonitoring()
{
    if (@available(iOS 14.0, *)) {
        s_hasHardwareKeyboard = (GCKeyboard.coalescedKeyboard != nil);

        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserverForName:GCKeyboardDidConnectNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *) {
                            s_hasHardwareKeyboard = true;
                        }];
        [center addObserverForName:GCKeyboardDidDisconnectNotification
                            object:nil
                             queue:[NSOperationQueue mainQueue]
                        usingBlock:^(NSNotification *) {
                            if (@available(iOS 14.0, *))
                                s_hasHardwareKeyboard = (GCKeyboard.coalescedKeyboard != nil);
                            else
                                s_hasHardwareKeyboard = false;
                        }];
    }
}

bool iosHasHardwareKeyboard()
{
    return s_hasHardwareKeyboard;
}
