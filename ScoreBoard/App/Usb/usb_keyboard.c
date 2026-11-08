//--------------------------------------------------------------
// File     : usb_keyboard.c
// USB keyboard on the OTG_HS port, built on the CubeMX USB Host HID class.
//
// MX_USB_HOST_Init() (called by CubeMX in the default task) starts the host
// stack, which runs in its own FreeRTOS thread. Key reports arrive through
// USBH_HID_EventCallback() and are queued until the App task reads them.
//--------------------------------------------------------------
#include "usb_keyboard.h"
#include "usb_host.h"
#include "usbh_hid.h"

extern USBH_HandleTypeDef hUsbHostHS;
extern ApplicationTypeDef Appli_state;

/* Key queue: written by the USB host task, read by the App task (single
 * producer / single consumer, no lock needed). */
#define KEY_QUEUE_SIZE 16u
static uint8_t key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t key_head;   /* written by the USB task */
static volatile uint8_t key_tail;   /* written by the App task */

void UsbKbd_Init(void)
{
  /* The host stack is started by MX_USB_HOST_Init(). */
  key_tail = key_head;
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
  if (key_tail == key_head)
  {
    return 0;
  }
  uint8_t key = key_queue[key_tail];
  key_tail = (uint8_t)((key_tail + 1u) % KEY_QUEUE_SIZE);
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
      uint8_t code = info->keys[0];          /* HID usage code */
      uint8_t key;
      if (code >= 0x3Au && code <= 0x45u)    /* F1..F12 */
      {
        key = (uint8_t)(KB_F1 + (code - 0x3Au));
      }
      else if (code >= 0x4Fu && code <= 0x52u)   /* right, left, down, up */
      {
        key = (uint8_t)(KB_RIGHT + (code - 0x4Fu));
      }
      else if (code == 0x29u)
      {
        key = KB_ESC;
      }
      else
      {
        key = USBH_HID_GetASCIICode(info);
      }
      uint8_t next = (uint8_t)((key_head + 1u) % KEY_QUEUE_SIZE);
      if (key != 0U && next != key_tail)   /* drop the key if the queue is full */
      {
        key_queue[key_head] = key;
        key_head = next;
      }
    }
  }
}
