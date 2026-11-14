"""ScoreBoard control panel (CustomTkinter): USB serial or Bluetooth LE.

    python -m scoreboard.gui        (from the tools folder)

On connect the board clock is set to this PC's time (can be turned off).
Every board command runs on a worker thread, so the window never freezes.
"""
from __future__ import annotations

import datetime as dt
import queue
import re
import threading
import traceback

import customtkinter as ctk

from .client import BAND_ITEMS, COLOURS, MODES, SCENES, SCREENS, CommandError, Message, ScoreBoard, quote
from .link import BleLink, LinkError, SerialLink, ble_scan, serial_ports
from .preview import CHARS, PreviewData, render_message, render_screen

PREVIEW_SCALE = 5
STATUS_EVERY_MS = 5000
SCREEN_TITLES = {"clock": "Clock", "countdown": "Countdown", "countup": "Count-up", "end": "Countdown end"}


def _big(days: int, hms: str) -> str:
    """Big digits as the firmware shows them (ClockCalc_FormatBig)."""
    h, m = hms.split(":")[:2]
    if days == 0:
        return hms
    if days < 1000:
        return f"{days}D {h}:{m}"
    return f"{days}D {h}H"


# ---------------------------------------------------------------- worker thread

class Worker:
    """Runs board commands one after the other off the UI thread."""

    def __init__(self, app: ctk.CTk) -> None:
        self.app = app
        self.q: "queue.Queue" = queue.Queue()
        threading.Thread(target=self._loop, daemon=True).start()

    def submit(self, fn, done=None, fail=None) -> None:
        self.q.put((fn, done, fail))

    def _loop(self) -> None:
        while True:
            fn, done, fail = self.q.get()
            try:
                result = fn()
            except Exception as e:  # noqa: BLE001 - reported in the status bar
                if not isinstance(e, (CommandError, LinkError)):
                    traceback.print_exc()
                self.app.after(0, (fail or self.app.show_error), e)
                continue
            if done is not None:
                self.app.after(0, done, result)

    @property
    def busy(self) -> bool:
        return not self.q.empty()


# ---------------------------------------------------------------- the window

class App(ctk.CTk):
    def __init__(self) -> None:
        super().__init__()
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")
        self.title("ScoreBoard")
        self.geometry("1240x800")
        self.minsize(1100, 720)

        self.worker = Worker(self)
        self.sb: ScoreBoard | None = None
        self.ble_devices = []
        self.status: dict[str, str] = {}
        self.info: dict[str, str] = {}
        self.messages: list[Message] = []
        self.msg_settings: dict[str, str] = {}
        self.screens: dict[str, dict] = {}
        self.players: list[tuple[int, str]] = []
        self.cd: dict[str, str] = {}
        self.cu: dict[str, str] = {}
        self.editing_id: int | None = None
        self.rows = 64              # until the board says otherwise (info: matrix=128x64)
        self.top_fault = False      # info: top_fault=1 on the old board

        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)
        self._build_sidebar()
        self._build_tabs()
        self.statusbar = ctk.CTkLabel(self, text="Not connected", anchor="w")
        self.statusbar.grid(row=1, column=0, columnspan=2, sticky="ew", padx=10, pady=(0, 6))
        self._set_connected(False)
        self.after(300, self.scan)
        self.after(STATUS_EVERY_MS, self._poll_status)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # ============================================================ helpers

    def show_error(self, e: Exception) -> None:
        self.statusbar.configure(text=f"⚠ {e}", text_color="#ff8a80")

    def note(self, text: str) -> None:
        self.statusbar.configure(text=text, text_color=("gray20", "gray80"))

    def run(self, cmd: str | list[str], done=None, note: str | None = None, refresh=None) -> None:
        """Run one or more commands on the board; done(last reply), then refresh()."""
        if self.sb is None:
            self.show_error(Exception("not connected"))
            return
        cmds = [cmd] if isinstance(cmd, str) else cmd

        def job():
            reply = None
            for c in cmds:
                reply = self.sb.run(c)
            return reply

        def ok(reply):
            if note:
                self.note(note)
            if done:
                done(reply)
            if refresh:
                refresh()

        self.worker.submit(job, ok)

    def _section(self, parent, title: str) -> ctk.CTkFrame:
        frame = ctk.CTkFrame(parent)
        frame.pack(fill="x", padx=8, pady=6)
        ctk.CTkLabel(frame, text=title, font=ctk.CTkFont(weight="bold")).grid(
            row=0, column=0, columnspan=8, sticky="w", padx=10, pady=(8, 4))
        return frame

    # ============================================================ sidebar

    def _build_sidebar(self) -> None:
        sb = ctk.CTkFrame(self, width=270, corner_radius=0)
        sb.grid(row=0, column=0, sticky="nsw")
        sb.grid_propagate(False)
        ctk.CTkLabel(sb, text="ScoreBoard", font=ctk.CTkFont(size=24, weight="bold")).pack(pady=(18, 10))

        self.transport = ctk.CTkSegmentedButton(sb, values=["Bluetooth", "USB serial"], command=lambda _v: self.scan())
        self.transport.set("Bluetooth")
        self.transport.pack(padx=14, fill="x")

        row = ctk.CTkFrame(sb, fg_color="transparent")
        row.pack(padx=14, pady=8, fill="x")
        self.device = ctk.CTkComboBox(row, values=[""], width=170)
        self.device.pack(side="left", fill="x", expand=True)
        self.scan_btn = ctk.CTkButton(row, text="Scan", width=60, command=self.scan)
        self.scan_btn.pack(side="left", padx=(6, 0))

        self.connect_btn = ctk.CTkButton(sb, text="Connect", command=self.toggle_connect, height=36)
        self.connect_btn.pack(padx=14, fill="x")

        self.sync_on_connect = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(sb, text="Set board time on connect (internet time)",
                        variable=self.sync_on_connect).pack(padx=14, pady=(12, 4), anchor="w")
        self.sync_btn = ctk.CTkButton(sb, text="Set board time now", command=self.sync_time)
        self.sync_btn.pack(padx=14, fill="x")

        box = ctk.CTkFrame(sb)
        box.pack(padx=14, pady=14, fill="x")
        self.info_labels = {}
        for key, title in [("time", "Board time"), ("mode", "Mode"), ("game", "Game"), ("score", "Score"),
                           ("brightness", "Brightness"), ("messages", "Messages"), ("ble", "Bluetooth"),
                           ("usb", "USB keyboard"), ("fw", "Firmware")]:
            r = ctk.CTkFrame(box, fg_color="transparent")
            r.pack(fill="x", padx=8, pady=1)
            ctk.CTkLabel(r, text=title, width=92, anchor="w", text_color="gray60").pack(side="left")
            lbl = ctk.CTkLabel(r, text="-", anchor="w")
            lbl.pack(side="left", fill="x", expand=True)
            self.info_labels[key] = lbl

    def scan(self) -> None:
        if self.sb is not None:
            return
        if self.transport.get() == "USB serial":
            ports = serial_ports()
            values = [f"{dev}  {desc}" for dev, desc in ports] or ["(no serial ports)"]
            self.device.configure(values=values)
            self.device.set(values[0])
            self.note(f"{len(ports)} serial port(s)")
            return
        self.scan_btn.configure(state="disabled", text="...")
        self.note("Scanning for Bluetooth devices (5 s)...")

        def done(devs):
            self.ble_devices = devs
            values = [f"{d.name}  {d.address}" for d in devs] or ["(not found)"]
            self.device.configure(values=values)
            self.device.set(values[0])
            self.scan_btn.configure(state="normal", text="Scan")
            self.note(f"{len(devs)} scoreboard(s) found" if devs else
                      "No scoreboard found: powered? phone still connected?")

        def fail(e):
            self.scan_btn.configure(state="normal", text="Scan")
            self.show_error(e)

        self.worker.submit(ble_scan, done, fail)

    def toggle_connect(self) -> None:
        if self.sb is not None:
            sb, self.sb = self.sb, None
            self.worker.submit(sb.close)
            self._set_connected(False)
            self.note("Disconnected")
            return
        choice = self.device.get()
        if self.transport.get() == "USB serial":
            if not choice or choice.startswith("("):
                return self.show_error(Exception("choose a serial port"))
            link = SerialLink(choice.split()[0])
        else:
            dev = next((d for d in self.ble_devices if d.address in choice), None)
            if dev is None:
                return self.show_error(Exception("scan and choose the scoreboard first"))
            link = BleLink(dev.address, dev.name)
        sb = ScoreBoard(link)
        self.connect_btn.configure(state="disabled", text="Connecting...")
        self.note(f"Connecting to {link.name}...")

        def job():
            sb.connect()
            synced = sb.sync_time() if self.sync_on_connect.get() else None
            return synced

        def done(synced):
            self.sb = sb
            self._set_connected(True)
            self.note(f"Connected to {link.name}" + (", " + self._sync_text(synced) if synced else ""))
            self.refresh_all()

        def fail(e):
            self._set_connected(False)
            self.show_error(e)
            try:
                link.close()
            except Exception:
                pass

        self.worker.submit(job, done, fail)

    def _set_connected(self, on: bool) -> None:
        self.connect_btn.configure(state="normal", text="Disconnect" if on else "Connect")
        state = "disabled" if on else "normal"
        self.transport.configure(state=state)
        self.device.configure(state=state)
        self.scan_btn.configure(state=state)
        self.sync_btn.configure(state="normal" if on else "disabled")
        if not on:
            for lbl in self.info_labels.values():
                lbl.configure(text="-")

    @staticmethod
    def _sync_text(synced) -> str:
        text, source, pc_error = synced
        msg = f"board time set to {text} ({source})"
        if pc_error is not None and abs(pc_error) >= 5:
            msg += f" - this PC's clock is {abs(pc_error) / 60:.1f} min {'fast' if pc_error > 0 else 'slow'}"
        return msg

    def sync_time(self) -> None:
        if self.sb is None:
            return
        self.worker.submit(self.sb.sync_time,
                           lambda r: (self.note(self._sync_text(r)[0].upper() + self._sync_text(r)[1:]), self.refresh_status()))

    def _on_close(self) -> None:
        if self.sb is not None:
            try:
                self.sb.close()
            except Exception:
                pass
        self.destroy()

    # ============================================================ refresh

    def refresh_all(self) -> None:
        def job():
            return {
                "info": self.sb.info(), "status": self.sb.status(), "msgs": self.sb.messages(),
                "screens": self.sb.screens(), "players": self.sb.run("game players").lines,
                "cd": self.sb.run("cd").kv(), "cu": self.sb.run("cu").kv(),
                "after": self.sb.run("cd after").kv(),
            }

        def done(r):
            self.info = r["info"]
            self.rows = 64 if self.info.get("matrix", "").endswith("x64") else 32
            self.top_fault = self.info.get("top_fault", "0") == "1"
            self._msg_preview()
            self._apply_status(r["status"])
            self.messages, self.msg_settings = r["msgs"]
            self.screens = r["screens"]
            self.players = []
            for line in r["players"]:
                m = re.match(r"player=(\d+) name=(.*)", line)
                if m:
                    self.players.append((int(m.group(1)), m.group(2).strip()))
            self.cd, self.cu = r["cd"], r["cu"]
            self._fill_after(r["after"])
            self._fill_game()
            self._fill_messages()
            self._fill_screen_form()
            self._fill_timers()
            self._fill_settings()

        self.worker.submit(job, done)

    def refresh_status(self) -> None:
        if self.sb is not None:
            self.worker.submit(self.sb.status, self._apply_status)

    def _poll_status(self) -> None:
        if self.sb is not None and not self.worker.busy:
            self.worker.submit(self.sb.status, self._apply_status, lambda e: None)
        self.after(STATUS_EVERY_MS, self._poll_status)

    def _apply_status(self, st: dict) -> None:
        self.status = st
        values = dict(st)
        values["fw"] = self.info.get("fw", "-")
        for key, lbl in self.info_labels.items():
            lbl.configure(text=values.get(key, "-"))
        if "brightness" in st and not self._slider_dragging:
            try:
                self.bright.set(int(st["brightness"]))
                self.bright_lbl.configure(text=f"{st['brightness']} %")
            except ValueError:
                pass
        self.score_lbl.configure(text=st.get("score", "0:0").replace(":", "  :  "))
        self._update_screen_preview()

    # ============================================================ tabs

    def _build_tabs(self) -> None:
        self.tabs = ctk.CTkTabview(self)
        self.tabs.grid(row=0, column=1, sticky="nsew", padx=(6, 10), pady=(4, 0))
        for name in ["Display", "Game", "Countdown", "Count-up", "Messages", "Screens", "Settings", "Console"]:
            self.tabs.add(name)
        self._slider_dragging = False
        self._build_display(self.tabs.tab("Display"))
        self._build_game(self.tabs.tab("Game"))
        self._build_countdown(self.tabs.tab("Countdown"))
        self._build_countup(self.tabs.tab("Count-up"))
        self._build_messages(self.tabs.tab("Messages"))
        self._build_screens(self.tabs.tab("Screens"))
        self._build_settings(self.tabs.tab("Settings"))
        self._build_console(self.tabs.tab("Console"))

    # ------------------------------------------------------------ Display

    def _build_display(self, tab) -> None:
        f = self._section(tab, "Show on the board")
        titles = {"pingpong": "Ping-pong", "clock": "Clock", "countdown": "Countdown", "countup": "Count-up",
                  "messages": "Messages"}
        for i, mode in enumerate(MODES):
            ctk.CTkButton(f, text=titles[mode], width=150, height=44,
                          command=lambda m=mode: self.run(f"mode {m}", note=f"Showing {titles[m]}",
                                                          refresh=self.refresh_status)
                          ).grid(row=1, column=i, padx=6, pady=(4, 12))

        ctk.CTkButton(f, text="Display test (30 s)", width=150, height=44, fg_color="gray30",
                      command=lambda: self.run("test", note="Display test running (any key stops it)")
                      ).grid(row=2, column=0, padx=6, pady=(0, 12))

        f = self._section(tab, "Brightness")
        self.bright = ctk.CTkSlider(f, from_=5, to=100, number_of_steps=19, width=520,
                                    command=lambda v: self.bright_lbl.configure(text=f"{int(v)} %"))
        self.bright.grid(row=1, column=0, padx=12, pady=12)
        self.bright.bind("<ButtonPress-1>", lambda _e: setattr(self, "_slider_dragging", True))
        self.bright.bind("<ButtonRelease-1>", self._bright_release)
        self.bright_lbl = ctk.CTkLabel(f, text="- %", width=60)
        self.bright_lbl.grid(row=1, column=1)

        f = self._section(tab, "Remote keyboard (keys of the shown mode)")
        pad = [("F12 menu", "f12", 1, 0), ("Up", "up", 1, 2), ("Enter", "enter", 1, 4),
               ("Left", "left", 2, 1), ("Down", "down", 2, 2), ("Right", "right", 2, 3),
               ("Esc", "esc", 2, 4), ("Del", "del", 2, 0)]
        for text, key, r, c in pad:
            ctk.CTkButton(f, text=text, width=90, command=lambda k=key: self.run(f"key {k}")).grid(
                row=r, column=c, padx=4, pady=4)
        self.key_text = ctk.CTkEntry(f, placeholder_text="type letters / digits, e.g. T or 2026", width=300)
        self.key_text.grid(row=3, column=0, columnspan=3, padx=6, pady=(4, 12), sticky="w")
        ctk.CTkButton(f, text="Send keys", width=90,
                      command=lambda: self.key_text.get() and self.run(f"key {quote(self.key_text.get())}")
                      ).grid(row=3, column=3, padx=4, pady=(4, 12))

    def _bright_release(self, _e) -> None:
        self._slider_dragging = False
        value = int(self.bright.get())
        self.run(f"bright {value}", note=f"Brightness {value} %")

    # ------------------------------------------------------------ Game

    def _build_game(self, tab) -> None:
        f = self._section(tab, "Players")
        ctk.CTkLabel(f, text="Side 1").grid(row=1, column=0, padx=10)
        self.p1 = ctk.CTkOptionMenu(f, values=["-"], width=200)
        self.p1.grid(row=1, column=1, padx=6, pady=8)
        ctk.CTkLabel(f, text="Side 2").grid(row=1, column=2, padx=10)
        self.p2 = ctk.CTkOptionMenu(f, values=["-"], width=200)
        self.p2.grid(row=1, column=3, padx=6, pady=8)
        ctk.CTkButton(f, text="Show ping-pong", command=lambda: self.run("mode pingpong", refresh=self.refresh_status)
                      ).grid(row=2, column=0, columnspan=2, padx=6, pady=(4, 12), sticky="w")
        ctk.CTkButton(f, text="Select players", command=self._game_select).grid(row=2, column=2, padx=6, pady=(4, 12))
        ctk.CTkButton(f, text="Start game", fg_color="#2e7d32", hover_color="#1b5e20",
                      command=lambda: self._game("game start", "Game started")).grid(row=2, column=3, padx=6, pady=(4, 12))

        f = self._section(tab, "Score")
        self.score_lbl = ctk.CTkLabel(f, text="0  :  0", font=ctk.CTkFont(size=64, weight="bold"))
        self.score_lbl.grid(row=1, column=0, columnspan=4, pady=8)
        for side, col in ((1, 0), (2, 2)):
            ctk.CTkButton(f, text=f"+1  side {side}", width=160, height=50, font=ctk.CTkFont(size=18),
                          command=lambda s=side: self._game(f"game point {s}")).grid(row=2, column=col, padx=8, pady=6)
            ctk.CTkButton(f, text="undo", width=80, fg_color="gray30",
                          command=lambda s=side: self._game(f"game undo {s}")).grid(row=2, column=col + 1, padx=8, pady=6)
        self.game_state = ctk.CTkLabel(f, text="", text_color="gray60")
        self.game_state.grid(row=3, column=0, columnspan=4, pady=(0, 10))

    def _fill_game(self) -> None:
        names = [f"{i}  {n}" for i, n in self.players] or ["-"]
        self.p1.configure(values=names)
        self.p2.configure(values=names)
        self.p1.set(names[1] if len(names) > 1 else names[0])
        self.p2.set(names[2] if len(names) > 2 else names[0])

    def _game_select(self) -> None:
        try:
            a, b = int(self.p1.get().split()[0]), int(self.p2.get().split()[0])
        except (ValueError, IndexError):
            return self.show_error(Exception("choose two players"))
        self._game(f"game select {a} {b}", "Players selected")

    def _game(self, cmd: str, note: str | None = None) -> None:
        def done(r):
            kv = r.kv()
            self.score_lbl.configure(text=kv.get("score", "0:0").replace(":", "  :  "))
            self.game_state.configure(text=f"{kv.get('state', '')}   {kv.get('player1', '')}  vs  {kv.get('player2', '')}")
        self.run(cmd, done, note)

    # ------------------------------------------------------------ Countdown

    def _build_countdown(self, tab) -> None:
        f = self._section(tab, "Count down to")
        ctk.CTkLabel(f, text="Date").grid(row=1, column=0, padx=(10, 4))
        self.cd_date = ctk.CTkEntry(f, placeholder_text="YYYY-MM-DD", width=120)
        self.cd_date.grid(row=1, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="Time").grid(row=1, column=2, padx=(10, 4))
        self.cd_time = ctk.CTkEntry(f, placeholder_text="HH:MM[:SS]", width=100)
        self.cd_time.grid(row=1, column=3, padx=4)
        ctk.CTkButton(f, text="Set", width=80, command=self._cd_set).grid(row=1, column=4, padx=8)
        ctk.CTkLabel(f, text="or for").grid(row=2, column=0, padx=(10, 4))
        self.cd_in = ctk.CTkEntry(f, placeholder_text="1h30m, 2d, 45s", width=120)
        self.cd_in.grid(row=2, column=1, padx=4, pady=(4, 10))
        ctk.CTkButton(f, text="Start", width=80, command=lambda: self.cd_in.get() and self.run(
            f"cd in {self.cd_in.get().strip()}", note="Countdown started", refresh=self._load_timers)
                      ).grid(row=2, column=2, padx=8, pady=(4, 10))
        self.cd_info = ctk.CTkLabel(f, text="", text_color="gray60")
        self.cd_info.grid(row=2, column=3, columnspan=3, sticky="w", padx=8)

        f = self._section(tab, "Texts")
        ctk.CTkLabel(f, text="Label (with the days)").grid(row=1, column=0, sticky="w", padx=10)
        self.cd_label = ctk.CTkEntry(f, width=380)
        self.cd_label.grid(row=1, column=1, padx=6, pady=4)
        ctk.CTkLabel(f, text="Text at zero").grid(row=2, column=0, sticky="w", padx=10)
        self.cd_end = ctk.CTkEntry(f, width=380)
        self.cd_end.grid(row=2, column=1, padx=6, pady=4)
        self.cd_days = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(f, text="Also show the days with the label (49D LABEL)", variable=self.cd_days,
                        command=lambda: self.run(f"cd days {'on' if self.cd_days.get() else 'off'}",
                                                 note="Countdown label setting saved", refresh=self._load_timers)
                        ).grid(row=3, column=1, sticky="w", padx=6, pady=(4, 10))
        ctk.CTkButton(f, text="Save texts", command=lambda: self.run(
            [f"cd label {quote(self.cd_label.get())}", f"cd end {quote(self.cd_end.get())}"],
            note="Countdown texts saved", refresh=self._load_timers)).grid(row=1, column=2, rowspan=2, padx=10)

        f = self._section(tab, "At zero")
        ctk.CTkLabel(f, text="Scene").grid(row=1, column=0, padx=(10, 4))
        self.cd_scene = ctk.CTkOptionMenu(f, values=SCENES, width=130)
        self.cd_scene.grid(row=1, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="for").grid(row=1, column=2, padx=4)
        self.cd_scene_s = ctk.CTkEntry(f, width=70)
        self.cd_scene_s.insert(0, "30")
        self.cd_scene_s.grid(row=1, column=3, padx=4)
        self.cd_scene_inf = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(f, text="seconds  /  infinite", variable=self.cd_scene_inf).grid(row=1, column=4, padx=6)
        ctk.CTkLabel(f, text="then").grid(row=2, column=0, padx=(10, 4))
        self.cd_then = ctk.CTkOptionMenu(f, values=["hold", "screen"], width=130)
        self.cd_then.grid(row=2, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="(hold = 00:00:00 + text, screen = 'Countdown end' layout)", text_color="gray60"
                     ).grid(row=2, column=2, columnspan=4, sticky="w", padx=4)
        ctk.CTkButton(f, text="Save", command=self._cd_after).grid(row=3, column=0, columnspan=2, padx=10, pady=(4, 12), sticky="w")
        ctk.CTkButton(f, text="Clear the end show", fg_color="gray30", command=lambda: self.run(
            "cd dismiss", note="End show cleared")).grid(row=3, column=2, columnspan=2, pady=(4, 12))

        f = self._section(tab, "")
        ctk.CTkButton(f, text="Show countdown", command=lambda: self.run("mode countdown", refresh=self.refresh_status)
                      ).grid(row=1, column=0, padx=10, pady=10)
        ctk.CTkButton(f, text="Remove countdown", fg_color="#8e2c2c", hover_color="#6d1f1f", command=lambda: self.run(
            "cd clear", note="Countdown removed", refresh=self._load_timers)).grid(row=1, column=1, padx=10, pady=10)

    def _fill_after(self, kv: dict) -> None:
        """'after=fireworks 30s, then hold' / 'after=stars infinite' / 'after=no scene, then screen'"""
        text = kv.get("after", "")
        m = re.match(r"(\w+)", text)
        scene = m.group(1) if m and m.group(1) in SCENES else "none"
        self.cd_scene.set(scene)
        self.cd_scene_inf.set("infinite" in text)
        s = re.search(r"(\d+)s", text)
        if s:
            self.cd_scene_s.delete(0, "end")
            self.cd_scene_s.insert(0, s.group(1))
        t = re.search(r"then (\w+)", text)
        if t:
            self.cd_then.set(t.group(1))

    def _cd_set(self) -> None:
        date, time_ = self.cd_date.get().strip(), self.cd_time.get().strip()
        if not time_:
            return self.show_error(Exception("enter a time (HH:MM or HH:MM:SS)"))
        self.run(f"cd set {date} {time_}".replace("  ", " "), note="Countdown set", refresh=self._load_timers)

    def _cd_after(self) -> None:
        secs = "inf" if self.cd_scene_inf.get() else (self.cd_scene_s.get().strip() or "30")
        self.run([f"cd after scene {self.cd_scene.get()} {secs}", f"cd after then {self.cd_then.get()}"],
                 note="End of countdown saved", refresh=self._load_timers)

    # ------------------------------------------------------------ Count-up

    def _build_countup(self, tab) -> None:
        f = self._section(tab, "Count up")
        ctk.CTkButton(f, text="Start now", width=120, command=lambda: self.run(
            "cu start", note="Count-up started", refresh=self._load_timers)).grid(row=1, column=0, padx=10, pady=8)
        ctk.CTkLabel(f, text="or from").grid(row=1, column=1, padx=4)
        self.cu_date = ctk.CTkEntry(f, placeholder_text="YYYY-MM-DD", width=120)
        self.cu_date.grid(row=1, column=2, padx=4)
        self.cu_time = ctk.CTkEntry(f, placeholder_text="HH:MM[:SS]", width=100)
        self.cu_time.grid(row=1, column=3, padx=4)
        ctk.CTkButton(f, text="Start", width=80, command=lambda: self.run(
            f"cu start {self.cu_date.get().strip()} {self.cu_time.get().strip()}".strip(),
            note="Count-up started", refresh=self._load_timers)).grid(row=1, column=4, padx=8)
        ctk.CTkButton(f, text="Stop", width=100, fg_color="gray30", command=lambda: self.run(
            "cu stop", refresh=self._load_timers)).grid(row=2, column=0, padx=10, pady=(4, 10))
        ctk.CTkButton(f, text="Resume", width=100, fg_color="gray30", command=lambda: self.run(
            "cu resume", refresh=self._load_timers)).grid(row=2, column=1, padx=4, pady=(4, 10))
        self.cu_info = ctk.CTkLabel(f, text="", text_color="gray60")
        self.cu_info.grid(row=2, column=2, columnspan=3, sticky="w", padx=8)

        f = self._section(tab, "Label")
        self.cu_label = ctk.CTkEntry(f, width=380)
        self.cu_label.grid(row=1, column=0, padx=10, pady=(4, 12))
        ctk.CTkButton(f, text="Save", command=lambda: self.run(
            f"cu label {quote(self.cu_label.get())}", note="Label saved", refresh=self._load_timers)
                      ).grid(row=1, column=1, padx=6, pady=(4, 12))
        self.cu_days = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(f, text="Also show the days with the label", variable=self.cu_days,
                        command=lambda: self.run(f"cu days {'on' if self.cu_days.get() else 'off'}",
                                                 note="Count-up label setting saved", refresh=self._load_timers)
                        ).grid(row=2, column=0, sticky="w", padx=10, pady=(0, 10))

        f = self._section(tab, "")
        ctk.CTkButton(f, text="Show count-up", command=lambda: self.run("mode countup", refresh=self.refresh_status)
                      ).grid(row=1, column=0, padx=10, pady=10)
        ctk.CTkButton(f, text="Remove count-up", fg_color="#8e2c2c", hover_color="#6d1f1f", command=lambda: self.run(
            "cu clear", note="Count-up removed", refresh=self._load_timers)).grid(row=1, column=1, padx=10, pady=10)

    def _load_timers(self) -> None:
        def job():
            return self.sb.run("cd").kv(), self.sb.run("cu").kv()

        def done(r):
            self.cd, self.cu = r
            self._fill_timers()
            self._update_screen_preview()
        self.worker.submit(job, done)

    def _fill_timers(self) -> None:
        cd, cu = self.cd, self.cu
        self.cd_info.configure(text=f"target {cd.get('target', '-')}   remaining {cd.get('remaining', '-')}")
        for entry, value in ((self.cd_label, cd.get("label", "")), (self.cd_end, cd.get("end", "")),
                             (self.cu_label, cu.get("label", ""))):
            entry.delete(0, "end")
            entry.insert(0, value)
        target = cd.get("target", "none")
        if target != "none" and " " in target:
            d, t = target.split(" ", 1)
            for entry, value in ((self.cd_date, d), (self.cd_time, t)):
                entry.delete(0, "end")
                entry.insert(0, value)
        self.cd_days.set(cd.get("days", "off") == "on")
        self.cu_days.set(cu.get("days", "off") == "on")
        self.cu_info.configure(text=f"start {cu.get('start', '-')}   elapsed {cu.get('elapsed', '-')}   {cu.get('state', '')}")

    # ------------------------------------------------------------ Messages

    def _build_messages(self, tab) -> None:
        tab.grid_columnconfigure(0, weight=1)
        tab.grid_columnconfigure(1, weight=1)
        tab.grid_rowconfigure(0, weight=1)

        left = ctk.CTkFrame(tab)
        left.grid(row=0, column=0, sticky="nsew", padx=(4, 6), pady=4)
        ctk.CTkLabel(left, text="Messages (play order)", font=ctk.CTkFont(weight="bold")).pack(anchor="w", padx=10, pady=(8, 2))
        self.msg_list = ctk.CTkScrollableFrame(left)
        self.msg_list.pack(fill="both", expand=True, padx=6, pady=6)
        self.msg_count = ctk.CTkLabel(left, text="", text_color="gray60")
        self.msg_count.pack(anchor="w", padx=10)

        s = ctk.CTkFrame(left)
        s.pack(fill="x", padx=6, pady=6)
        ctk.CTkLabel(s, text="Order").grid(row=0, column=0, padx=6, pady=4)
        self.msg_order = ctk.CTkOptionMenu(s, values=["seq", "random"], width=100)
        self.msg_order.grid(row=0, column=1, padx=4)
        ctk.CTkLabel(s, text="Show (s)").grid(row=0, column=2, padx=6)
        self.msg_time = ctk.CTkEntry(s, width=50)
        self.msg_time.grid(row=0, column=3, padx=4)
        ctk.CTkLabel(s, text="Gap (s)").grid(row=1, column=0, padx=6, pady=4)
        self.msg_gap = ctk.CTkEntry(s, width=60)
        self.msg_gap.grid(row=1, column=1, padx=4, sticky="w")
        self.msg_ticker = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(s, text="Ticker", variable=self.msg_ticker).grid(row=1, column=2, columnspan=2, padx=6)
        ctk.CTkButton(s, text="Save settings", command=self._msg_settings_save).grid(
            row=2, column=0, columnspan=4, padx=6, pady=(4, 8), sticky="w")

        right = ctk.CTkFrame(tab)
        right.grid(row=0, column=1, sticky="nsew", padx=(6, 4), pady=4)
        self.msg_editor_title = ctk.CTkLabel(right, text="New message", font=ctk.CTkFont(weight="bold"))
        self.msg_editor_title.pack(anchor="w", padx=10, pady=(8, 4))
        self.msg_l1 = ctk.CTkEntry(right, placeholder_text="Line 1 (up to 40 characters)")
        self.msg_l1.pack(fill="x", padx=10, pady=4)
        self.msg_l2 = ctk.CTkEntry(right, placeholder_text="Line 2 (optional)")
        self.msg_l2.pack(fill="x", padx=10, pady=4)
        self.msg_hint = ctk.CTkLabel(right, text="", text_color="gray60", justify="left")
        self.msg_hint.pack(anchor="w", padx=10)
        row = ctk.CTkFrame(right, fg_color="transparent")
        row.pack(fill="x", padx=10, pady=6)
        ctk.CTkLabel(row, text="Colour").pack(side="left")
        self.msg_colour = ctk.CTkOptionMenu(row, values=COLOURS, width=120, command=lambda _v: self._msg_preview())
        self.msg_colour.pack(side="left", padx=8)
        self.msg_save_btn = ctk.CTkButton(row, text="Add", width=90, command=self._msg_save)
        self.msg_save_btn.pack(side="left", padx=4)
        ctk.CTkButton(row, text="New", width=70, fg_color="gray30", command=self._msg_new).pack(side="left", padx=4)
        for e in (self.msg_l1, self.msg_l2):
            e.bind("<KeyRelease>", lambda _e: self._msg_preview())
        ctk.CTkLabel(right, text="Preview (Messages mode, as on the board):", text_color="gray60").pack(anchor="w", padx=10, pady=(8, 0))
        self.msg_preview = ctk.CTkLabel(right, text="")
        self.msg_preview.pack(padx=10, pady=6)
        self._msg_preview()

    def _fill_messages(self) -> None:
        for w in self.msg_list.winfo_children():
            w.destroy()
        for m in self.messages:
            row = ctk.CTkFrame(self.msg_list)
            row.pack(fill="x", pady=2)
            swatch = {"white": "#ffffff", "red": "#ff3333", "green": "#33ff33", "blue": "#3366ff",
                      "yellow": "#ffff33", "cyan": "#33ffff", "magenta": "#ff33ff"}.get(m.colour, "#ffffff")
            ctk.CTkLabel(row, text=f"{m.id:>2}", width=26).pack(side="left", padx=(6, 2))
            ctk.CTkLabel(row, text="●", text_color=swatch, width=16).pack(side="left")
            text = m.line1 + (f"  /  {m.line2}" if m.line2 else "")
            ctk.CTkLabel(row, text=text, anchor="w").pack(side="left", fill="x", expand=True, padx=6)
            ctk.CTkButton(row, text="Del", width=44, fg_color="#8e2c2c", hover_color="#6d1f1f",
                          command=lambda i=m.id: self.run(f"msg del {i}", note=f"Message {i} deleted",
                                                          refresh=self._load_messages)).pack(side="right", padx=2)
            ctk.CTkButton(row, text="Show", width=50, fg_color="gray30",
                          command=lambda i=m.id: self.run(f"msg show {i}", refresh=self.refresh_status)).pack(side="right", padx=2)
            ctk.CTkButton(row, text="Edit", width=44, command=lambda msg=m: self._msg_edit(msg)).pack(side="right", padx=2)
        st = self.msg_settings
        self.msg_count.configure(text=f"{st.get('count', '?')} used")
        self.msg_order.set(st.get("order", "seq"))
        for entry, key in ((self.msg_time, "time"), (self.msg_gap, "gap")):
            entry.delete(0, "end")
            entry.insert(0, st.get(key, ""))
        self.msg_ticker.set(st.get("ticker", "on") == "on")

    def _load_messages(self) -> None:
        def done(r):
            self.messages, self.msg_settings = r
            self._fill_messages()
            self._update_screen_preview()
        self.worker.submit(self.sb.messages, done)

    def _msg_new(self) -> None:
        self.editing_id = None
        self.msg_editor_title.configure(text="New message")
        self.msg_save_btn.configure(text="Add")
        for e in (self.msg_l1, self.msg_l2):
            e.delete(0, "end")
        self._msg_preview()

    def _msg_edit(self, m: Message) -> None:
        self.editing_id = m.id
        self.msg_editor_title.configure(text=f"Message {m.id}")
        self.msg_save_btn.configure(text="Save")
        self.msg_l1.delete(0, "end")
        self.msg_l1.insert(0, m.line1)
        self.msg_l2.delete(0, "end")
        self.msg_l2.insert(0, m.line2)
        self.msg_colour.set(m.colour)
        self._msg_preview()

    def _msg_save(self) -> None:
        l1, l2, colour = self.msg_l1.get(), self.msg_l2.get(), self.msg_colour.get()
        if not l1.strip():
            return self.show_error(Exception("line 1 is empty"))
        args = f"{quote(l1)} {quote(l2) + ' ' if l2 else ''}{colour}"
        if self.editing_id is None:
            self.run(f"msg add {args}", note="Message added", refresh=self._load_messages)
            self._msg_new()
        else:
            self.run(f"msg edit {self.editing_id} {args}", note=f"Message {self.editing_id} saved", refresh=self._load_messages)

    def _msg_settings_save(self) -> None:
        cmds = [f"msg order {self.msg_order.get()}"]
        if self.msg_time.get().strip():
            cmds.append(f"msg time {self.msg_time.get().strip()}")
        if self.msg_gap.get().strip():
            cmds.append(f"msg gap {self.msg_gap.get().strip()}")
        cmds.append(f"msg ticker {'on' if self.msg_ticker.get() else 'off'}")
        self.run(cmds, note="Message settings saved", refresh=self._load_messages)

    def _msg_preview(self) -> None:
        l1, l2 = self.msg_l1.get(), self.msg_l2.get()
        hints = []
        for n, text in ((1, l1), (2, l2)):
            if text:
                state = "fits" if len(text) <= CHARS else "scrolls"
                hints.append(f"line {n}: {len(text)}/40 characters, {state}")
        if any(len(t) > 40 for t in (l1, l2)):
            hints.append("too long: 40 characters per line")
        self.msg_hint.configure(text="\n".join(hints))
        img = render_message(l1, l2, self.msg_colour.get(), self.rows, self.top_fault).image(
            PREVIEW_SCALE, self.top_fault)
        self._msg_img = ctk.CTkImage(light_image=img, dark_image=img, size=img.size)
        self.msg_preview.configure(image=self._msg_img)

    # ------------------------------------------------------------ Screens

    def _build_screens(self, tab) -> None:
        top = ctk.CTkFrame(tab, fg_color="transparent")
        top.pack(fill="x", padx=8, pady=6)
        ctk.CTkLabel(top, text="Screen").pack(side="left", padx=(4, 6))
        self.scr_name = ctk.CTkSegmentedButton(top, values=[SCREEN_TITLES[s] for s in SCREENS],
                                               command=lambda _v: self._fill_screen_form())
        self.scr_name.set(SCREEN_TITLES["clock"])
        self.scr_name.pack(side="left")

        f = ctk.CTkFrame(tab)
        f.pack(fill="x", padx=8, pady=4)
        heads = ["Line", "Shows", "Split", "Lower row", "Background"]
        for c, h in enumerate(heads):
            ctk.CTkLabel(f, text=h, text_color="gray60").grid(row=0, column=c, padx=8, pady=(6, 2), sticky="w")
        self.scr_rows = []
        half_items = [i for i in BAND_ITEMS if i != "big"]
        for i in range(4):
            lbl = ctk.CTkLabel(f, text=f"{i + 1}", width=40)
            lbl.grid(row=i + 1, column=0, padx=8, pady=3)
            top_item = ctk.CTkOptionMenu(f, values=BAND_ITEMS, width=120, command=lambda _v: self._update_screen_preview())
            top_item.grid(row=i + 1, column=1, padx=6, pady=3)
            split = ctk.BooleanVar(value=False)
            ctk.CTkCheckBox(f, text="", variable=split, width=30, command=self._split_changed).grid(row=i + 1, column=2, padx=6)
            bottom = ctk.CTkOptionMenu(f, values=half_items, width=120, command=lambda _v: self._update_screen_preview())
            bottom.grid(row=i + 1, column=3, padx=6, pady=3)
            bg = ctk.CTkOptionMenu(f, values=["black"] + COLOURS, width=110, command=lambda _v: self._update_screen_preview())
            bg.grid(row=i + 1, column=4, padx=6, pady=3)
            self.scr_rows.append((lbl, top_item, split, bottom, bg))
        ctk.CTkLabel(f, text="Messages on this screen").grid(row=5, column=0, columnspan=2, padx=8, pady=(8, 8), sticky="w")
        self.scr_msgs = ctk.CTkEntry(f, width=160, placeholder_text="all / none / 1 3 5")
        self.scr_msgs.grid(row=5, column=2, columnspan=2, padx=6, sticky="w")
        self.scr_msgs.bind("<KeyRelease>", lambda _e: self._update_screen_preview())
        ctk.CTkLabel(f, text="Big digits colour").grid(row=6, column=0, columnspan=2, padx=8, pady=(0, 8), sticky="w")
        self.scr_big = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_big.grid(row=6, column=2, columnspan=2, padx=6, pady=(0, 8), sticky="w")
        ctk.CTkLabel(f, text="Text colour").grid(row=7, column=0, columnspan=2, padx=8, pady=(0, 8), sticky="w")
        self.scr_text = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_text.grid(row=7, column=2, columnspan=2, padx=6, pady=(0, 8), sticky="w")
        ctk.CTkLabel(f, text="Hourglass colour").grid(row=8, column=0, columnspan=2, padx=8, pady=(0, 8), sticky="w")
        self.scr_sand = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_sand.grid(row=8, column=2, columnspan=2, padx=6, pady=(0, 8), sticky="w")
        ctk.CTkButton(f, text="Apply to the board", command=self._screen_apply).grid(row=5, column=4, padx=8)

        self.scr_note = ctk.CTkLabel(tab, text="", text_color="gray60", justify="left")
        self.scr_note.pack(anchor="w", padx=12)
        self.scr_preview = ctk.CTkLabel(tab, text="")
        self.scr_preview.pack(padx=8, pady=6)

    def _screen_key(self) -> str:
        title = self.scr_name.get()
        return next(k for k, v in SCREEN_TITLES.items() if v == title)

    def _fill_screen_form(self) -> None:
        cfg = self.screens.get(self._screen_key())
        if not cfg:
            self._update_screen_preview()
            return
        for i, (_lbl, top, split, bottom, bg) in enumerate(self.scr_rows):
            parts = cfg["lines"][i].split("/")
            top.set(parts[0])
            split.set(len(parts) == 2)
            bottom.set(parts[1] if len(parts) == 2 else "text")
            bg.set(cfg["bg"][i])
        self.scr_msgs.delete(0, "end")
        self.scr_msgs.insert(0, cfg["msgs"].replace(",", " "))
        self.scr_big.set(cfg.get("big", "auto"))
        self.scr_text.set(cfg.get("text", "auto"))
        self.scr_sand.set(cfg.get("sand", "auto"))
        self._split_changed()

    def _split_changed(self) -> None:
        for _lbl, top, split, bottom, _bg in self.scr_rows:
            bottom.configure(state="normal" if split.get() else "disabled")
            if split.get() and top.get() == "big":
                top.set("time")
        self._update_screen_preview()

    def _screen_layout(self) -> dict:
        lines, bgs = [], []
        for _lbl, top, split, bottom, bg in self.scr_rows:
            lines.append(f"{top.get()}/{bottom.get()}" if split.get() else top.get())
            bgs.append(bg.get())
        return {"lines": lines, "bg": bgs}

    def _screen_apply(self) -> None:
        s = self._screen_key()
        layout = self._screen_layout()
        cmds = [f"screen {s} {' '.join(layout['lines'])}"]
        cmds += [f"screen {s} {i + 1} bg {c}" for i, c in enumerate(layout["bg"])]
        msgs = self.scr_msgs.get().strip() or "all"
        cmds.append(f"screen {s} msgs {msgs}")
        cmds.append(f"screen {s} colour big {self.scr_big.get()}")
        cmds.append(f"screen {s} colour text {self.scr_text.get()}")
        cmds.append(f"screen {s} colour hourglass {self.scr_sand.get()}")

        def reload():
            self.worker.submit(self.sb.screens, lambda r: (setattr(self, "screens", r), self._fill_screen_form()))
        self.run(cmds, note=f"{SCREEN_TITLES[s]} screen saved", refresh=reload)

    def _preview_data(self, screen: str) -> PreviewData:
        t = self.status.get("time", "")
        big = t.split(" ")[1] if " " in t else "12:34:56"
        text = ""
        if screen == "clock":
            try:
                d = dt.date.fromisoformat(t.split(" ")[0])
                text = f"{d.strftime('%a').upper()} {d.isoformat()}"
            except ValueError:
                text = "SUN 2026-11-09"
        elif screen == "countdown":
            rem = self.cd.get("remaining", "0d 00:00:00")
            m = re.match(r"(\d+)d (\S+)", rem)
            label = self.cd.get("label", "")
            if m:
                days = int(m.group(1))
                big = _big(days, m.group(2))
                text = (f"{days}D {label}" if label else f"{days} DAYS") if days and self.cd.get("days", "off") == "on" else label
        elif screen == "countup":
            m = re.match(r"(\d+)d (\S+)", self.cu.get("elapsed", ""))
            label = self.cu.get("label", "")
            if m:
                days = int(m.group(1))
                big = _big(days, m.group(2))
                text = (f"{days}D {label}" if label else f"{days} DAYS") if days and self.cu.get("days", "off") == "on" else label
        else:
            big = "00:00:00"
            text = self.cd.get("end", "") or self.cd.get("label", "")
        want = self.scr_msgs.get().strip() if hasattr(self, "scr_msgs") else "all"
        msgs = self.messages
        if want == "none":
            msgs = []
        elif want and want != "all":
            ids = {int(x) for x in re.findall(r"\d+", want)}
            msgs = [m for m in msgs if m.id in ids]
        return PreviewData(big=big, text=text, messages=[(m.line1, m.line2, m.colour) for m in msgs],
                           big_colour=self.scr_big.get() if hasattr(self, "scr_big") else "auto",
                           text_colour=self.scr_text.get() if hasattr(self, "scr_text") else "auto",
                           sand_colour=self.scr_sand.get() if hasattr(self, "scr_sand") else "auto")

    def _update_screen_preview(self) -> None:
        if not hasattr(self, "scr_rows"):
            return
        screen = self._screen_key()
        img = render_screen(self._screen_layout(), self._preview_data(screen), self.rows, self.top_fault).image(
            PREVIEW_SCALE, self.top_fault)
        self._scr_img = ctk.CTkImage(light_image=img, dark_image=img, size=img.size)
        self.scr_preview.configure(image=self._scr_img)
        note = "Lines 2 and 4 stay dark on this board (HUB75 E line not wired)." if self.rows == 32 else ""
        self.scr_note.configure(text=note)

    # ------------------------------------------------------------ Settings

    def _build_settings(self, tab) -> None:
        f = self._section(tab, "Clock")
        self.trim_var = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(f, text="RTC accuracy trim (measured against the crystal)", variable=self.trim_var,
                        command=lambda: self.run(f"time trim {'on' if self.trim_var.get() else 'off'}",
                                                 note="RTC trim changed")).grid(row=1, column=0, padx=10, pady=(4, 10), sticky="w")

        f = self._section(tab, "Bluetooth (change while connected over USB serial)")
        ctk.CTkLabel(f, text="Name").grid(row=1, column=0, padx=10)
        self.ble_name = ctk.CTkEntry(f, width=160)
        self.ble_name.grid(row=1, column=1, padx=4, pady=4)
        ctk.CTkButton(f, text="Set", width=60, command=lambda: self.run(
            f"ble name {self.ble_name.get().strip()}", note="Bluetooth name set")).grid(row=1, column=2, padx=6)
        ctk.CTkLabel(f, text="PIN").grid(row=2, column=0, padx=10)
        self.ble_pin = ctk.CTkEntry(f, width=160, placeholder_text="6 digits or off")
        self.ble_pin.grid(row=2, column=1, padx=4, pady=(4, 10))
        ctk.CTkButton(f, text="Set", width=60, command=lambda: self.run(
            f"ble pin {self.ble_pin.get().strip()}", note="Bluetooth PIN set")).grid(row=2, column=2, padx=6, pady=(4, 10))

        f = self._section(tab, "Board")
        ctk.CTkButton(f, text="Reload everything", command=self.refresh_all).grid(row=1, column=0, padx=10, pady=(4, 10))
        ctk.CTkButton(f, text="Restart the board", fg_color="#8e2c2c", hover_color="#6d1f1f",
                      command=self._reboot).grid(row=1, column=1, padx=10, pady=(4, 10))
        self.board_info = ctk.CTkLabel(f, text="", justify="left", text_color="gray60")
        self.board_info.grid(row=2, column=0, columnspan=3, padx=10, pady=(0, 10), sticky="w")

    def _fill_settings(self) -> None:
        self.board_info.configure(text="   ".join(f"{k}: {v}" for k, v in self.info.items()))

    def _reboot(self) -> None:
        if self.sb is None:
            return
        sb, self.sb = self.sb, None
        def job():
            try:
                sb.link.write(b"reboot\r\n")
            finally:
                sb.link.close()
        self.worker.submit(job)
        self._set_connected(False)
        self.note("Board restarting; connect again in a few seconds")

    # ------------------------------------------------------------ Console

    def _build_console(self, tab) -> None:
        self.con_out = ctk.CTkTextbox(tab, font=ctk.CTkFont(family="Consolas", size=13))
        self.con_out.pack(fill="both", expand=True, padx=8, pady=(8, 4))
        row = ctk.CTkFrame(tab, fg_color="transparent")
        row.pack(fill="x", padx=8, pady=(0, 8))
        self.con_in = ctk.CTkEntry(row, placeholder_text="command, e.g. help, status, msg list")
        self.con_in.pack(side="left", fill="x", expand=True)
        self.con_in.bind("<Return>", lambda _e: self._console_send())
        ctk.CTkButton(row, text="Send", width=80, command=self._console_send).pack(side="left", padx=(6, 0))

    def _console_send(self) -> None:
        cmd = self.con_in.get().strip()
        if not cmd or self.sb is None:
            return
        self.con_in.delete(0, "end")
        self.con_out.insert("end", f"> {cmd}\n")

        def done(r):
            text = "\n".join(r.lines) + ("\n" if r.lines else "") + ("OK" if r.ok else f"ERR {r.error}") + "\n"
            self.con_out.insert("end", text)
            self.con_out.see("end")
        self.worker.submit(lambda: self.sb.command(cmd, timeout=10.0), done)


def main() -> None:
    App().mainloop()


if __name__ == "__main__":
    main()
