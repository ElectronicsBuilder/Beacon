"""Interactive terminal for the scoreboard console over Bluetooth LE or USB serial.

    python -m scoreboard.console              # find "Beacon" over BLE
    python -m scoreboard.console --port COM5  # USB serial adapter
    python -m scoreboard.console --scan       # list BLE devices and exit

Type commands as on the board's console (help, status, msg list ...).
Ctrl-C or "exit" quits. The board echoes what you type, so the line you
type shows up twice in this window.
"""
from __future__ import annotations

import argparse
import sys
import threading
import time

from .link import BleLink, LinkError, SerialLink, ble_scan, serial_ports


def _printer(link, stop: threading.Event) -> None:
    while not stop.is_set():
        data = link.read(timeout=0.1)
        if data:
            text = data.decode("ascii", "replace").replace("\r", "")
            text = text.replace("\x1b[K", "")        # line clear sequence
            sys.stdout.write(text)
            sys.stdout.flush()


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="USB serial port (e.g. COM5); default: Bluetooth")
    ap.add_argument("--address", help="BLE address (default: first device named Beacon/ScoreBoard/HMSoft)")
    ap.add_argument("--scan", action="store_true", help="list BLE devices and serial ports, then exit")
    args = ap.parse_args(argv)

    if args.scan:
        print("Serial ports:")
        for dev, desc in serial_ports():
            print(f"  {dev:8} {desc}")
        print("Bluetooth LE (scanning 5 s):")
        for d in ble_scan():
            print(f"  {d.address}  {d.name}  rssi {d.rssi}")
        return 0

    if args.port:
        link = SerialLink(args.port)
    else:
        address, name = args.address, ""
        if not address:
            print("Looking for Beacon over Bluetooth...")
            found = ble_scan()
            if not found:
                print("Not found. Is it powered and not connected to a phone? (--scan lists devices)")
                return 1
            address, name = found[0].address, found[0].name
        link = BleLink(address, name)

    print(f"Connecting to {link.name}...")
    try:
        link.open()
    except LinkError as e:
        print(e)
        return 1
    print("Connected. Type 'help' (exit or Ctrl-C to quit).")

    stop = threading.Event()
    threading.Thread(target=_printer, args=(link, stop), daemon=True).start()
    link.write(b"echo on\r\n")                   # a GUI may have left machine mode on
    try:
        while True:
            line = input()
            if line.strip() in ("exit", "quit"):
                break
            link.write(line.encode("ascii", "replace") + b"\r\n")
            time.sleep(0.05 if sys.stdin.isatty() else 2.0)   # piped input: let each reply arrive
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        stop.set()
        link.close()
        print("\nDisconnected.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
