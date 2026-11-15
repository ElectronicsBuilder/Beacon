# Beacon PC tools

Control panel and terminal for **Beacon**, the LED display board (clock, countdowns,
messages, scenes, ping-pong score), over **Bluetooth LE**
(the board's HM-11 module) or a **USB serial** adapter on the console port.

## Install (once)

```
cd tools
python -m pip install -r requirements.txt
```

Python 3.10 or newer. Bluetooth needs a PC with Bluetooth 4.0+ (BLE).

## Control panel (GUI)

```
python -m scoreboard.gui        # or double-click beacon.pyw
```

1. Choose **Bluetooth** (Scan finds "Beacon", or the old name "ScoreBoard") or **USB serial** (COM port).
2. **Connect.** The board clock is set to this PC's time (unless unticked).
3. Pages (icons on the left, `Ctrl+1`..`8`; `Ctrl+B` hides the side bar):
   - **Display**: mode, brightness, refresh rate, remote keyboard.
   - **Screens**: what each display line shows, its colours and font size, with a preview.
   - **Countdown** and **Count-up**: times, texts, scene at zero
     (**Try the scene now** plays it on the board without touching the countdown).
   - **Messages**: add or edit messages, with a preview.
   - **Game**: players, start, points.
   - **Console**: any command.
   - **Settings**: clock trim, Bluetooth name and PIN, restart.

The previews use the firmware's own fonts (`scoreboard/font8x8.py`, the tiny 3x5
font in `preview.py`) and its layout rules.

## Windows program (Beacon.exe)

```
build_exe.bat                   # or: powershell -ExecutionPolicy Bypass -File build_exe.ps1
```

Builds `dist\Beacon.exe` (one file, about 20 MB, no Python needed on the PC that
runs it). It installs PyInstaller and the requirements first if needed.

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
