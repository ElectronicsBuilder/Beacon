/*
 * hm11.h - HM-11 Bluetooth LE module on UART7 (PE7 = RX from the module,
 * PE8 = TX to the module), used as a second console port.
 *
 * The module is a transparent serial link while a phone/PC is connected;
 * while not connected it takes AT commands (HMSoft firmware: no line ends,
 * replies like "OK+Set:..."). At boot the driver finds the baud rate, turns on
 * connect notifications (AT+NOTI1: "OK+CONN" / "OK+LOST") and renames a
 * factory "HMSoft" module to "ScoreBoard". Nothing is sent to the module
 * while no one is connected, so console output is never taken as AT commands.
 *
 * UART7 interrupt: received bytes go to a ring buffer (Ble_Poll in the App
 * task), sent bytes come from a ring buffer, so long replies don't block.
 */
#ifndef APP_HM11_H
#define APP_HM11_H

#include <stdint.h>
#include <stddef.h>

typedef enum { BLE_ABSENT, BLE_IDLE, BLE_CONNECTED } BleState;

void Ble_Init(void);                       /* App_Init: probe and set up the module */
void Ble_Poll(void);                       /* App task: received bytes -> console */
void APP_UART7_IRQ(void);                  /* from UART7_IRQHandler */

BleState Ble_State(void);
const char *Ble_StateName(void);
const char *Ble_Name(void);
uint32_t Ble_Baud(void);
const char *Ble_Pin(void);                 /* "" = no PIN */

/* AT settings (only while no one is connected); 0 = ok */
int Ble_SetName(const char *name);         /* 1..12 letters, digits, - _ */
int Ble_SetPin(const char *pin);           /* 6 digits, or NULL = no PIN */

#endif
