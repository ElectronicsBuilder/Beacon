//--------------------------------------------------------------
// File     : usb_keyboard.c
// USB keyboard on the OTG_HS port, built on the CubeMX USB Host HID class.
//
// MX_USB_HOST_Init() (called by CubeMX in the default task) starts the host
// stack, which runs in its own FreeRTOS thread. Key reports arrive through
// USBH_HID_EventCallback(); the last ASCII key is kept until it is read.
//--------------------------------------------------------------
#include "usb_keyboard.h"
#include "usb_host.h"
#include "usbh_hid.h"

extern USBH_HandleTypeDef hUsbHostHS;
extern ApplicationTypeDef Appli_state;

static volatile uint8_t usb_last_key = 0;

void UsbKbd_Init(void)
{
  /* The host stack is started by MX_USB_HOST_Init(). */
  usb_last_key = 0;
}

USB_HID_HOST_STATUS_t UsbKbd_Do(void)
{
  if (hUsbHostHS.device.is_connected == 0U)
  {
    return USB_HID_DEV_DETACHED;
  }
  if (Appli_state != APPLICATION_READY)
  {
    return USB_HID_DEV_CONNECTED;
  }
  switch (USBH_HID_GetDeviceType(&hUsbHostHS))
  {
    case HID_KEYBOARD: return USB_HID_KEYBOARD_CONNECTED;
    case HID_MOUSE:    return USB_HID_MOUSE_CONNECTED;
    default:           return USB_HID_DEV_NOT_SUPPORTED;
  }
}

uint8_t UsbKbd_GetKey(void)
{
  uint8_t key = usb_last_key;
  usb_last_key = 0;
  return key;
}

/* Called by the HID class (USB host thread) for every input report. */
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{
  if (USBH_HID_GetDeviceType(phost) == HID_KEYBOARD)
  {
    HID_KEYBD_Info_TypeDef *info = USBH_HID_GetKeybdInfo(phost);
    if (info != NULL)
    {
      uint8_t key = USBH_HID_GetASCIICode(info);
      if (key != 0U)
      {
        usb_last_key = key;
      }
    }
  }
}
