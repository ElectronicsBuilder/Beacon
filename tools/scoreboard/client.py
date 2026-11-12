"""Command client for the scoreboard console (USB serial or BLE).

The board's console answers every command with data lines and a final
"OK" or "ERR <reason>" line. The client switches its port to machine mode
("echo off": no echo, no prompt, no log lines) while connected.

    sb = ScoreBoard(SerialLink("COM5"))
    sb.connect()
    print(sb.status()["time"])
    sb.sync_time()
    sb.close()
"""
from __future__ import annotations

import datetime as _dt
import re
import threading
import time
from dataclasses import dataclass, field

from .link import Link, LinkError

COLOURS = ["white", "red", "green", "blue", "yellow", "cyan", "magenta"]
SCREENS = ["clock", "countdown", "countup", "end"]
BAND_ITEMS = ["off", "big", "time", "text", "ticker", "msg"]
SCENES = ["none", "fireworks", "stars", "confetti", "snow", "rainbow"]
MODES = ["pingpong", "clock", "countdown", "countup", "messages"]


class CommandError(Exception):
    pass


@dataclass
class Reply:
    ok: bool
    lines: list[str] = field(default_factory=list)
    error: str = ""

    def kv(self) -> dict[str, str]:
        """key=value data lines as a dict (first '=' splits)."""
        out = {}
        for line in self.lines:
            if "=" in line:
                k, v = line.split("=", 1)
                out[k.strip()] = v.strip()
        return out


@dataclass
class Message:
    id: int
    colour: str
    line1: str
    line2: str = ""


def quote(text: str) -> str:
    """Text as one console argument: "..." with \\" and \\\\ escaped."""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


class ScoreBoard:
    def __init__(self, link: Link) -> None:
        self.link = link
        self._buf = ""
        self._lock = threading.Lock()

    # ------------------------------------------------------------ connection

    def connect(self) -> None:
        self.link.open()
        time.sleep(0.3)
        self._drain(0.5)                       # greeting / prompt
        self.link.write(b"\x03")               # Ctrl-C: clear a half-typed line
        self._drain(0.2)
        r = self.command("echo off", timeout=4.0)
        if not r.ok:
            raise CommandError("the board did not switch to machine mode")

    def close(self) -> None:
        try:
            if self.link.is_open:
                self.link.write(b"echo on\r\n")   # leave the port in typing mode
                time.sleep(0.2)
        except LinkError:
            pass
        self.link.close()

    @property
    def connected(self) -> bool:
        return self.link.is_open

    def _drain(self, seconds: float) -> None:
        end = time.time() + seconds
        while time.time() < end:
            self.link.read(timeout=0.05)
        self._buf = ""

    # ------------------------------------------------------------ commands

    def command(self, cmd: str, timeout: float = 6.0) -> Reply:
        """Run one console command; returns its data lines and OK/ERR."""
        with self._lock:
            self._buf = ""
            self.link.read()                   # drop anything old
            self.link.write(cmd.encode("ascii", "replace") + b"\r\n")
            lines: list[str] = []
            end = time.time() + timeout
            while time.time() < end:
                data = self.link.read(timeout=0.1)
                if data:
                    self._buf += data.decode("ascii", "replace")
                    end = max(end, time.time() + 1.0)    # still talking: wait a bit more
                while "\n" in self._buf:
                    line, self._buf = self._buf.split("\n", 1)
                    line = line.strip("\r").strip()
                    if not line or line.startswith("[") or line == cmd or line.startswith("> "):
                        continue                      # empty, log, echo, prompt
                    if line == "OK":
                        return Reply(True, lines)
                    if line.startswith("ERR"):
                        return Reply(False, lines, line[3:].strip())
                    lines.append(line)
            raise CommandError(f"no answer to '{cmd}'")

    def run(self, cmd: str, timeout: float = 6.0) -> Reply:
        """command(), raising CommandError on ERR."""
        r = self.command(cmd, timeout)
        if not r.ok:
            raise CommandError(f"{cmd}: {r.error}")
        return r

    # ------------------------------------------------------------ helpers

    def info(self) -> dict[str, str]:
        return self.run("info").kv()

    def status(self) -> dict[str, str]:
        return self.run("status").kv()

    def sync_time(self) -> tuple[str, str, float | None]:
        """Set the board clock to local time from internet time (NTP), or from
        this PC's clock without internet. Returns (time sent, source, PC clock
        error in seconds or None)."""
        from .ntp import local_now
        reference, source, pc_error = local_now()
        offset = reference - _dt.datetime.now()          # corrects a wrong PC clock
        now = _dt.datetime.now() + offset
        time.sleep((1_000_000 - now.microsecond) / 1e6)  # send on a whole second
        now = (_dt.datetime.now() + offset).replace(microsecond=0)
        text = now.strftime("%Y-%m-%d %H:%M:%S")
        self.run(f"time {text}")
        return text, source, pc_error

    def messages(self) -> tuple[list[Message], dict[str, str]]:
        """Message list in play order, and the settings line (count, order...)."""
        r = self.run("msg list")
        msgs, settings = [], {}
        pat = re.compile(r'^(\d+)\s+(\w+)\s+"((?:[^"\\]|\\.)*)"(?:\s+"((?:[^"\\]|\\.)*)")?$')
        for line in r.lines:
            m = pat.match(line)
            if m:
                unq = lambda s: (s or "").replace('\\"', '"').replace("\\\\", "\\")
                msgs.append(Message(int(m.group(1)), m.group(2), unq(m.group(3)), unq(m.group(4))))
            elif line.startswith("count="):
                for part in line.split():
                    if "=" in part:
                        k, v = part.split("=", 1)
                        settings[k] = v
        return msgs, settings

    def screens(self) -> dict[str, dict]:
        """screen -> {'lines': [item x4], 'bg': [colour x4], 'msgs': 'all'|...}"""
        r = self.run("screen")
        out = {}
        for line in r.lines:
            m = re.match(r"^(\w+):\s+(.*)$", line)
            if not m or m.group(1) not in SCREENS:
                continue
            items, bgs, msgs = ["off"] * 4, ["black"] * 4, "all"
            for tok in re.finditer(r"(\d)=([\w/]+)\*?(?:\(bg (\w+)\))?", m.group(2)):
                i = int(tok.group(1)) - 1
                items[i] = tok.group(2)
                if tok.group(3):
                    bgs[i] = tok.group(3)
            mm = re.search(r"msgs=(\S+)", m.group(2))
            if mm:
                msgs = mm.group(1)
            cb = re.search(r"big=(\w+)", m.group(2))
            ct = re.search(r"text=(\w+)", m.group(2))
            out[m.group(1)] = {"lines": items, "bg": bgs, "msgs": msgs,
                               "big": cb.group(1) if cb else "auto", "text": ct.group(1) if ct else "auto"}
        return out
