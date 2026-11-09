# ScoreBoard PC tools

Control panel and terminal for the LED scoreboard, over **Bluetooth LE**
(the board's HM-11 module) or a **USB serial** adapter on the console port.

## Install (once)

```
cd tools
python -m pip install -r requirements.txt
```

Python 3.10 or newer. Bluetooth needs a PC with Bluetooth 4.0+ (BLE).

## Control panel (GUI)

```
python -m scoreboard.gui        # or double-click scoreboard_gui.pyw
```

1. Choose **Bluetooth** (Scan finds "ScoreBoard") or **USB serial** (COM port).
2. **Connect.** The board clock is set to this PC's time (unless unticked).
3. Tabs:
   - **Display**: mode, brightness, remote keyboard.
   - **Game**: players, start, points.
   - **Countdown** and **Count-up**: times, texts, scene at zero.
   - **Messages**: add or edit messages, with a preview.
   - **Screens**: what each display line shows, with a preview.
   - **Settings**: clock trim, Bluetooth name and PIN, restart.
   - **Console**: any command.

The previews use the firmware's own 8x8 font (`scoreboard/font8x8.py`) and its
layout rules, including this board's wiring: only display lines 1 and 3 light
up (HUB75 E line not wired), and lines 1-2 have blue stuck on.

## Terminal

```
python -m scoreboard.console              # finds the board over Bluetooth
python -m scoreboard.console --port COM5  # USB serial
python -m scoreboard.console --scan       # list devices
```

## Notes

- **Bluetooth LE devices don't show up in Windows Bluetooth settings** and get
  no COM port. Programs connect to them directly, as these tools do.
- **The module takes one connection at a time.** Disconnect the phone first.
- **The HM-11 is an HM-10-type module:**
  - service `FFE0`, characteristic `FFE1` (notify + write without response);
  - 9600 baud to the MCU;
  - writes are sent in 20-byte pieces.
- **Protocol:** the console's text commands (`help` on the board).
  - The client sends `echo off` (machine mode). Every command then answers with
    data lines (mostly `key=value`) and a final `OK` or `ERR <reason>`.
  - `echo on` is sent again on disconnect.
- **After a firmware font change:** run `python -m scoreboard.export_font`.
