/*
 * keys.h - key codes delivered to the display modes.
 * Printable keys are ASCII; Enter = '\r', Backspace = '\b', Delete = 0x7F.
 * Keys without an ASCII value use codes 0x80 and up.
 */
#ifndef APP_KEYS_H
#define APP_KEYS_H

#define KB_ENTER    '\r'
#define KB_BACK     '\b'
#define KB_DEL      0x7Fu
#define KB_ESC      0x1Bu
#define KB_F1       0x81u          /* F1..F12 = 0x81..0x8C */
#define KB_F8       0x88u
#define KB_F12      0x8Cu
#define KB_RIGHT    0x90u
#define KB_LEFT     0x91u
#define KB_DOWN     0x92u
#define KB_UP       0x93u

#endif
