//--------------------------------------------------------------
// File     : usb_keyboard.h
// USB keyboard on the OTG_HS port (internal FS PHY, PB14/PB15).
// Wraps the CubeMX USB Host HID class; keeps the status values and the
// "one key per call" behaviour of the original keyboard library.
//--------------------------------------------------------------
#ifndef APP_USB_KEYBOARD_H
#define APP_USB_KEYBOARD_H

#include <stdint.h>

typedef enum {
  USB_HID_HOST_NO_INIT = 0,   // USB not initialised yet
  USB_HID_DEV_DETACHED,       // no device attached
  USB_HID_SPEED_OK,           // (kept for compatibility)
  USB_HID_SPEED_ERROR,        // (kept for compatibility)
  USB_HID_DEV_NOT_SUPPORTED,  // device attached but not a keyboard/mouse
  USB_HID_OVER_CURRENT,       // (kept for compatibility)
  USB_HID_DEV_CONNECTED,      // device attached, enumeration running
  USB_HID_MOUSE_CONNECTED,    // mouse ready
  USB_HID_KEYBOARD_CONNECTED  // keyboard ready
} USB_HID_HOST_STATUS_t;

#include "keys.h"            /* KB_* key codes */

void UsbKbd_Init(void);
USB_HID_HOST_STATUS_t UsbKbd_Do(void);
uint8_t UsbKbd_GetKey(void);   // next queued key (ASCII or KB_*), 0 = none

#endif // APP_USB_KEYBOARD_H
