"""Internet time (SNTP, no extra packages), so the board is set right even
when this PC's clock is wrong.

    t = internet_time()          # aware datetime in UTC, or None (no internet)
"""
from __future__ import annotations

import datetime as dt
import socket
import struct
import time

SERVERS = ("pool.ntp.org", "time.windows.com", "time.google.com")
NTP_EPOCH = 2208988800          # seconds from 1900-01-01 to 1970-01-01


def _query(server: str, timeout: float) -> float:
    """UTC seconds since 1970 from one server, corrected for the round trip."""
    packet = b"\x1b" + 47 * b"\0"                     # LI 0, version 3, client mode
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(timeout)
        t0 = time.monotonic()
        s.sendto(packet, (server, 123))
        data, _ = s.recvfrom(48)
        t1 = time.monotonic()
    secs, frac = struct.unpack("!II", data[40:48])     # transmit timestamp
    server_time = secs - NTP_EPOCH + frac / 2**32
    return server_time + (t1 - t0) / 2 - (time.monotonic() - t1)   # now, at return


def internet_time(timeout: float = 2.0) -> dt.datetime | None:
    for server in SERVERS:
        try:
            utc = _query(server, timeout)
            return dt.datetime.fromtimestamp(utc, dt.timezone.utc)
        except OSError:
            continue
    return None


def local_now() -> tuple[dt.datetime, str, float | None]:
    """(local wall time, source, PC clock error in s or None).

    Internet time converted with this PC's time zone; the PC clock only when
    there is no internet."""
    net = internet_time()
    pc = dt.datetime.now().astimezone()
    if net is None:
        return pc.replace(tzinfo=None), "PC clock", None
    local = net.astimezone(pc.tzinfo)
    return local.replace(tzinfo=None), "internet time", (pc - local).total_seconds()
