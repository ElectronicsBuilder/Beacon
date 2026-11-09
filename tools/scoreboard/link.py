"""Byte links to the scoreboard: USB serial (pyserial) or Bluetooth LE (bleak).

Both deliver received bytes into a queue from a background thread, so the
client and the GUI use them the same way:

    link = SerialLink("COM5")            # or BleLink(address)
    link.open()
    link.write(b"status\\r\\n")
    data = link.read(timeout=0.5)        # bytes received so far (b"" if none)
    link.close()
"""
from __future__ import annotations

import asyncio
import queue
import threading
import time
from dataclasses import dataclass

# HM-10/HM-11 serial service and characteristic
BLE_SERVICE = "0000ffe0-0000-1000-8000-00805f9b34fb"
BLE_CHAR = "0000ffe1-0000-1000-8000-00805f9b34fb"
BLE_CHUNK = 20          # bytes per BLE write
BLE_CHUNK_GAP = 0.03    # s between writes: the module forwards them at 9600 baud


class LinkError(Exception):
    pass


class Link:
    """Base: received bytes go into self._rx (a queue of bytes objects)."""

    name = "link"

    def __init__(self) -> None:
        self._rx: "queue.Queue[bytes]" = queue.Queue()

    def open(self) -> None:
        raise NotImplementedError

    def close(self) -> None:
        raise NotImplementedError

    def write(self, data: bytes) -> None:
        raise NotImplementedError

    @property
    def is_open(self) -> bool:
        raise NotImplementedError

    def read(self, timeout: float = 0.0) -> bytes:
        """All bytes received so far; waits up to timeout for the first ones."""
        out = bytearray()
        try:
            out += self._rx.get(timeout=timeout) if timeout > 0 else self._rx.get_nowait()
        except queue.Empty:
            return b""
        while True:
            try:
                out += self._rx.get_nowait()
            except queue.Empty:
                return bytes(out)


# ---------------------------------------------------------------- USB serial

def serial_ports() -> list[tuple[str, str]]:
    """(device, description) of the serial ports on this PC."""
    from serial.tools import list_ports
    return [(p.device, p.description or p.device) for p in list_ports.comports()]


class SerialLink(Link):
    def __init__(self, port: str, baud: int = 115200) -> None:
        super().__init__()
        self.port = port
        self.baud = baud
        self.name = port
        self._ser = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()

    def open(self) -> None:
        import serial
        try:
            self._ser = serial.Serial(self.port, self.baud, timeout=0.05)
        except serial.SerialException as e:
            raise LinkError(f"cannot open {self.port}: {e}") from e
        self._stop.clear()
        self._thread = threading.Thread(target=self._reader, daemon=True)
        self._thread.start()

    def _reader(self) -> None:
        while not self._stop.is_set():
            try:
                data = self._ser.read(256)
            except Exception:
                break
            if data:
                self._rx.put(data)

    def write(self, data: bytes) -> None:
        if not self.is_open:
            raise LinkError("not connected")
        self._ser.write(data)

    def close(self) -> None:
        self._stop.set()
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
        self._ser = None

    @property
    def is_open(self) -> bool:
        return self._ser is not None and self._ser.is_open


# ---------------------------------------------------------------- Bluetooth LE

@dataclass
class BleDevice:
    address: str
    name: str
    rssi: int


def ble_scan(timeout: float = 5.0) -> list[BleDevice]:
    """BLE devices that look like an HM-1x (name ScoreBoard/HMSoft or FFE0 service)."""
    from bleak import BleakScanner

    async def scan():
        return await BleakScanner.discover(timeout=timeout, return_adv=True)

    found = asyncio.run(scan())
    result = []
    for addr, (dev, adv) in found.items():
        name = adv.local_name or dev.name or ""
        if "ScoreBoard" in name or "HMSoft" in name or BLE_SERVICE in [u.lower() for u in adv.service_uuids]:
            result.append(BleDevice(addr, name or addr, adv.rssi))
    return sorted(result, key=lambda d: -d.rssi)


class BleLink(Link):
    """HM-11 transparent serial over BLE. bleak runs on its own asyncio loop thread."""

    def __init__(self, address: str, name: str = "") -> None:
        super().__init__()
        self.address = address
        self.name = name or address
        self._loop = asyncio.new_event_loop()
        self._thread = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._client = None

    def _run(self, coro, timeout: float = 20.0):
        return asyncio.run_coroutine_threadsafe(coro, self._loop).result(timeout)

    def open(self) -> None:
        from bleak import BleakClient
        if not self._thread.is_alive():
            self._thread.start()

        async def connect():
            client = BleakClient(self.address, timeout=15.0, disconnected_callback=lambda _c: None)
            await client.connect()
            await client.start_notify(BLE_CHAR, lambda _h, d: self._rx.put(bytes(d)))
            return client

        try:
            self._client = self._run(connect())
        except Exception as e:
            raise LinkError(f"cannot connect to {self.name}: {e}") from e

    def write(self, data: bytes) -> None:
        if not self.is_open:
            raise LinkError("not connected")

        async def send():
            for i in range(0, len(data), BLE_CHUNK):
                await self._client.write_gatt_char(BLE_CHAR, data[i:i + BLE_CHUNK], response=False)
                await asyncio.sleep(BLE_CHUNK_GAP)

        self._run(send())

    def close(self) -> None:
        if self._client is not None:
            try:
                self._run(self._client.disconnect(), timeout=5.0)
            except Exception:
                pass
        self._client = None

    @property
    def is_open(self) -> bool:
        return self._client is not None and self._client.is_connected
