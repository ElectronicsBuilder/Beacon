"""Beacon control panel (CustomTkinter): USB serial or Bluetooth LE.

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
from . import theme
from .theme import C, ICON

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

class Tooltip:
    """VS Code style hover text."""

    def __init__(self, widget, text: str) -> None:
        self.widget, self.text, self.tip, self.job = widget, text, None, None
        widget.bind("<Enter>", self._schedule, add="+")
        widget.bind("<Leave>", self._hide, add="+")
        widget.bind("<ButtonPress>", self._hide, add="+")

    def _schedule(self, _e=None) -> None:
        self.job = self.widget.after(450, self._show)

    def _show(self) -> None:
        self.job = None
        if self.tip is not None:
            return
        x = self.widget.winfo_rootx() + self.widget.winfo_width() + 4
        y = self.widget.winfo_rooty() + self.widget.winfo_height() // 2 - 12
        self.tip = tw = ctk.CTkToplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.configure(fg_color=C["card"])
        ctk.CTkLabel(tw, text=self.text, font=theme.font(12), fg_color=C["card"], text_color=C["text"],
                     corner_radius=0).pack(padx=8, pady=3)
        tw.wm_geometry(f"+{x}+{y}")

    def _hide(self, _e=None) -> None:
        if self.job:
            self.widget.after_cancel(self.job)
            self.job = None
        if self.tip is not None:
            self.tip.destroy()
            self.tip = None


class ActivityItem(ctk.CTkFrame):
    """One icon of the activity bar (left edge), with the white line when active."""

    def __init__(self, parent, glyph: str, tip: str, command) -> None:
        super().__init__(parent, width=48, height=48, corner_radius=0, fg_color=C["panel"])
        self.grid_propagate(False)
        self.command = command
        self.bar = ctk.CTkFrame(self, width=2, height=48, corner_radius=0, fg_color=C["panel"])
        self.bar.place(x=0, y=0)
        self.lbl = ctk.CTkLabel(self, text=glyph, font=theme.icon_font(22), text_color=C["faint"],
                                width=46, height=48, fg_color="transparent")
        self.lbl.place(x=2, y=0)
        self.active = False
        for w in (self, self.lbl):
            w.bind("<Button-1>", lambda _e: self.command())
            w.bind("<Enter>", lambda _e: self._hover(True))
            w.bind("<Leave>", lambda _e: self._hover(False))
        Tooltip(self.lbl, tip)

    def _hover(self, on: bool) -> None:
        if not self.active:
            self.lbl.configure(text_color=C["text"] if on else C["faint"])

    def set_active(self, on: bool) -> None:
        self.active = on
        self.bar.configure(fg_color=C["text_bright"] if on else C["panel"])
        self.lbl.configure(text_color=C["text_bright"] if on else C["faint"])


# pages: key, title, icon, short description; console and settings sit at the bottom of the activity bar
PAGES = [
    ("display", "Display", "display", "What the board shows, brightness, refresh rate, remote keys"),
    ("screens", "Screens", "screens", "The 4 lines of the clock, countdown, count-up and end screens"),
    ("countdown", "Countdown", "countdown", "Count down to a date, the texts, what happens at zero"),
    ("countup", "Count-up", "countup", "Count up from now or from a date"),
    ("messages", "Messages", "messages", "The message list, its order and timing"),
    ("game", "Game", "game", "Ping-pong players and score"),
    ("console", "Console", "console", "Type board commands (help lists them)"),
    ("settings", "Settings", "settings", "Clock, Bluetooth, board"),
]
BOTTOM_PAGES = ("console", "settings")


class App(ctk.CTk):
    def __init__(self) -> None:
        theme.set_app_id()
        theme.apply_theme()
        super().__init__()
        self.title("Beacon")
        self.geometry("1320x860")
        self.minsize(1160, 720)
        theme.set_window_icon(self)

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
        self.page = None

        self.grid_columnconfigure(2, weight=1)
        self.grid_rowconfigure(0, weight=1)
        self._build_statusbar()
        self._build_sidebar()
        self._build_tabs()
        self._set_connected(False)
        self.show_page("display")
        self._bind_keys()
        self.after(300, self.scan)
        self.after(STATUS_EVERY_MS, self._poll_status)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _bind_keys(self) -> None:
        """VS Code like: Ctrl+1..8 pages, Ctrl+B side bar, Ctrl+` console."""
        for i, (key, *_rest) in enumerate(PAGES):
            self.bind_all(f"<Control-Key-{i + 1}>", lambda _e, k=key: self.show_page(k))
        self.bind_all("<Control-b>", lambda _e: self.toggle_sidebar())
        self.bind_all("<Control-grave>", lambda _e: self.show_page("console"))

    # ============================================================ helpers

    def show_error(self, e: Exception) -> None:
        self.statusbar.configure(text=f"\u26a0  {e}")
        self.status_frame.configure(fg_color=C["danger"])
        self.after(6000, self._status_colour)

    def note(self, text: str) -> None:
        self.statusbar.configure(text=text)
        self._status_colour()

    def _status_colour(self) -> None:
        self.status_frame.configure(fg_color=C["status_on"] if self.sb is not None else C["status_off"])

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
        """A card with a VS Code style section title (UPPER CASE, small, muted)."""
        frame = ctk.CTkFrame(parent, fg_color=C["card"], border_width=1, border_color=C["border"])
        frame.pack(fill="x", padx=14, pady=7)
        if title:
            ctk.CTkLabel(frame, text=title.upper(), font=theme.font(11, "bold"), text_color=C["muted"]).grid(
                row=0, column=0, columnspan=9, sticky="w", padx=12, pady=(10, 4))
        return frame

    def _icon_button(self, parent, glyph: str, text: str, command, kind: str = "primary", **kw) -> ctk.CTkButton:
        """Button with an icon glyph before the text."""
        colours = {"primary": (C["button"], C["button_hover"]), "secondary": (C["button2"], C["button2_hover"]),
                   "danger": (C["danger"], C["danger_hover"]), "ok": (C["ok"], C["ok_hover"]),
                   "ghost": ("transparent", C["hover"])}[kind]
        return ctk.CTkButton(parent, text=f"{glyph}  {text}" if text else glyph, command=command,
                             fg_color=colours[0], hover_color=colours[1], **kw)

    # ============================================================ sidebar

    def _side_header(self, parent, text: str) -> None:
        ctk.CTkLabel(parent, text=text.upper(), font=theme.font(11, "bold"), text_color=C["muted"],
                     anchor="w").pack(fill="x", padx=16, pady=(16, 6))

    def _activity_item(self, parent, key: str, title: str, icon: str) -> ActivityItem:
        n = [p[0] for p in PAGES].index(key) + 1
        item = ActivityItem(parent, ICON[icon], f"{title}  (Ctrl+{n})", lambda k=key: self.show_page(k))
        self.activity[key] = item
        return item

    def _build_sidebar(self) -> None:
        """Left: activity bar (page icons); next to it the side bar (connection + board status)."""
        act = ctk.CTkFrame(self, width=48, corner_radius=0, fg_color=C["panel"])
        act.grid(row=0, column=0, sticky="nsw")
        act.grid_propagate(False)
        act.grid_rowconfigure(len(PAGES), weight=1)
        self.activity: dict[str, ActivityItem] = {}
        row = 0
        for key, title, icon, _desc in PAGES:
            if key not in BOTTOM_PAGES:
                self._activity_item(act, key, title, icon).grid(row=row, column=0)
                row += 1
        row = len(PAGES) + 1
        for key, title, icon, _desc in PAGES:
            if key in BOTTOM_PAGES:
                self._activity_item(act, key, title, icon).grid(row=row, column=0)
                row += 1

        sb = ctk.CTkFrame(self, width=280, corner_radius=0, fg_color=C["panel"], border_width=0)
        sb.grid(row=0, column=1, sticky="nsw")
        sb.grid_propagate(False)
        sb.pack_propagate(False)
        self.sidebar = sb

        title = ctk.CTkFrame(sb, fg_color="transparent")
        title.pack(fill="x", padx=16, pady=(14, 0))
        from PIL import Image
        self._logo = ctk.CTkImage(Image.open(theme.ensure_icon().with_suffix(".png")), size=(22, 22))
        ctk.CTkLabel(title, text="", image=self._logo).pack(side="left")
        ctk.CTkLabel(title, text="BEACON", font=theme.font(12, "bold"), text_color=C["text"]).pack(side="left", padx=8)

        self._side_header(sb, "Connection")
        self.transport = ctk.CTkSegmentedButton(sb, values=["Bluetooth", "USB serial"], command=lambda _v: self.scan(),
                                                font=theme.font(12))
        self.transport.set("Bluetooth")
        self.transport.pack(padx=16, fill="x")

        row = ctk.CTkFrame(sb, fg_color="transparent")
        row.pack(padx=16, pady=8, fill="x")
        self.device = ctk.CTkComboBox(row, values=[""], width=200, font=theme.font(12))
        self.device.pack(side="left", fill="x", expand=True)
        self.scan_btn = ctk.CTkButton(row, text=ICON["refresh"], width=32, font=theme.icon_font(14),
                                      fg_color=C["button2"], hover_color=C["button2_hover"], command=self.scan)
        self.scan_btn.pack(side="left", padx=(6, 0))
        Tooltip(self.scan_btn, "Scan again")

        self.connect_btn = ctk.CTkButton(sb, text="Connect", command=self.toggle_connect, height=32)
        self.connect_btn.pack(padx=16, fill="x")

        self.sync_on_connect = ctk.BooleanVar(value=True)
        ctk.CTkCheckBox(sb, text="Set board time on connect", variable=self.sync_on_connect, font=theme.font(12),
                        checkbox_width=18, checkbox_height=18).pack(padx=16, pady=(12, 6), anchor="w")
        self.sync_btn = self._icon_button(sb, "", "Set board time now", self.sync_time, "secondary", height=30)
        self.sync_btn.pack(padx=16, fill="x")

        self._side_header(sb, "Board")
        box = ctk.CTkFrame(sb, fg_color="transparent")
        box.pack(padx=16, fill="x")
        box.grid_columnconfigure(1, weight=1)
        self.info_labels = {}
        for i, (key, title) in enumerate([("time", "Time"), ("mode", "Mode"), ("game", "Game"), ("score", "Score"),
                                          ("brightness", "Brightness"), ("refresh_hz", "Refresh"),
                                          ("messages", "Messages"), ("ble", "Bluetooth"), ("usb", "Keyboard"),
                                          ("fw", "Firmware")]):
            ctk.CTkLabel(box, text=title, anchor="w", text_color=C["muted"], font=theme.font(12),
                         height=22).grid(row=i, column=0, sticky="w")
            lbl = ctk.CTkLabel(box, text="-", anchor="w", font=theme.font(12), height=22)
            lbl.grid(row=i, column=1, sticky="w", padx=(12, 0))
            self.info_labels[key] = lbl

        ctk.CTkLabel(sb, text="Ctrl+1..8 pages    Ctrl+B side bar", font=theme.font(11),
                     text_color=C["faint"]).pack(side="bottom", anchor="w", padx=16, pady=10)

    def toggle_sidebar(self) -> None:
        if self.sidebar.winfo_ismapped():
            self.sidebar.grid_remove()
        else:
            self.sidebar.grid()

    def _build_statusbar(self) -> None:
        """Bottom bar: blue when connected (VS Code style)."""
        self.status_frame = ctk.CTkFrame(self, height=24, corner_radius=0, fg_color=C["status_off"])
        self.status_frame.grid(row=1, column=0, columnspan=3, sticky="ew")
        self.status_frame.grid_propagate(False)
        self.status_frame.grid_columnconfigure(2, weight=1)
        self.status_glyph = ctk.CTkLabel(self.status_frame, text="", font=theme.icon_font(12),
                                         text_color=C["text_bright"], height=24)
        self.status_glyph.grid(row=0, column=0, padx=(12, 4), sticky="w")
        self.status_conn = ctk.CTkLabel(self.status_frame, text="", font=theme.font(12), text_color=C["text_bright"],
                                        height=24)
        self.status_conn.grid(row=0, column=1, padx=(0, 18))
        self.statusbar = ctk.CTkLabel(self.status_frame, text="Not connected", anchor="w", font=theme.font(12),
                                      text_color=C["text_bright"], height=24)
        self.statusbar.grid(row=0, column=2, sticky="ew")
        self.status_right = ctk.CTkLabel(self.status_frame, text="", font=theme.font(12), text_color=C["text_bright"],
                                         height=24)
        self.status_right.grid(row=0, column=3, padx=10)

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
        self.scan_btn.configure(state="disabled")
        self.note("Scanning for Bluetooth devices (5 s)...")

        def done(devs):
            self.ble_devices = devs
            values = [f"{d.name}  {d.address}" for d in devs] or ["(not found)"]
            self.device.configure(values=values)
            self.device.set(values[0])
            self.scan_btn.configure(state="normal")
            self.note(f"{len(devs)} board(s) found" if devs else
                      "No board found: powered? phone still connected?")

        def fail(e):
            self.scan_btn.configure(state="normal")
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
                return self.show_error(Exception("scan and choose the board first"))
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
        self.connect_btn.configure(state="normal", text="Disconnect" if on else "Connect",
                                   fg_color=C["button2"] if on else C["button"],
                                   hover_color=C["button2_hover"] if on else C["button_hover"])
        self.status_glyph.configure(text=ICON["bluetooth"] if self.transport.get() == "Bluetooth" else ICON["usb"])
        self.status_conn.configure(text=self.sb.link.name if on and self.sb is not None else "not connected")
        self._status_colour()
        if not on:
            self.status_right.configure(text="")
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
                "led": self.sb.run("led").kv(),
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
            self._fill_leds(r["led"])

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
        values["refresh_hz"] = f"{st['refresh_hz']} Hz" if "refresh_hz" in st else "-"
        for key, lbl in self.info_labels.items():
            lbl.configure(text=values.get(key, "-"))
        right = [st.get("time", ""), st.get("mode", ""), f"{st['brightness']} %" if "brightness" in st else ""]
        self.status_right.configure(text="      ".join(x for x in right if x))
        if "brightness" in st and not self._slider_dragging:
            try:
                self.bright.set(int(st["brightness"]))
                self.bright_lbl.configure(text=f"{st['brightness']} %")
            except ValueError:
                pass
        if "refresh_hz" in st and not self._slider_dragging:
            self._show_refresh(st)
        self.score_lbl.configure(text=st.get("score", "0:0").replace(":", "  :  "))
        self._update_screen_preview()

    # ============================================================ tabs

    def _build_tabs(self) -> None:
        """Right: a tab strip with the page name and its actions, breadcrumbs, then the page."""
        main = ctk.CTkFrame(self, corner_radius=0, fg_color=C["bg"])
        main.grid(row=0, column=2, sticky="nsew")
        main.grid_columnconfigure(0, weight=1)
        main.grid_rowconfigure(2, weight=1)

        strip = ctk.CTkFrame(main, height=36, corner_radius=0, fg_color=C["panel"])
        strip.grid(row=0, column=0, sticky="ew")
        strip.grid_propagate(False)
        strip.grid_columnconfigure(1, weight=1)
        tab = ctk.CTkFrame(strip, corner_radius=0, fg_color=C["bg"], height=36)
        tab.grid(row=0, column=0, sticky="nsw")
        ctk.CTkFrame(tab, height=2, corner_radius=0, fg_color=C["accent"]).pack(fill="x", side="top")
        self.tab_icon = ctk.CTkLabel(tab, text="", font=theme.icon_font(14), text_color=C["accent"], height=32)
        self.tab_icon.pack(side="left", padx=(14, 8))
        self.tab_title = ctk.CTkLabel(tab, text="", font=theme.font(13), text_color=C["text_bright"], height=32)
        self.tab_title.pack(side="left", padx=(0, 18))
        self.tab_actions = ctk.CTkFrame(strip, fg_color="transparent")
        self.tab_actions.grid(row=0, column=2, sticky="e", padx=8)

        self.crumb = ctk.CTkLabel(main, text="", font=theme.font(12), text_color=C["muted"], anchor="w", height=30)
        self.crumb.grid(row=1, column=0, sticky="ew", padx=16, pady=(4, 0))

        holder = ctk.CTkFrame(main, corner_radius=0, fg_color=C["bg"])
        holder.grid(row=2, column=0, sticky="nsew")
        holder.grid_columnconfigure(0, weight=1)
        holder.grid_rowconfigure(0, weight=1)
        self.pages: dict[str, ctk.CTkFrame] = {}
        for key, *_rest in PAGES:
            if key in ("messages", "console"):        # these fill the height themselves
                frame = ctk.CTkFrame(holder, corner_radius=0, fg_color=C["bg"])
            else:
                frame = ctk.CTkScrollableFrame(holder, corner_radius=0, fg_color=C["bg"])
            self.pages[key] = frame

        self._slider_dragging = False
        self._build_display(self.pages["display"])
        self._build_game(self.pages["game"])
        self._build_countdown(self.pages["countdown"])
        self._build_countup(self.pages["countup"])
        self._build_messages(self.pages["messages"])
        self._build_screens(self.pages["screens"])
        self._build_settings(self.pages["settings"])
        self._build_console(self.pages["console"])
        self.after(100, self._update_screen_preview)

    def _page_actions(self, key: str) -> list:
        """Buttons in the tab strip, like VS Code's editor actions."""
        def show(mode: str, what: str):
            return (ICON["play"], f"Show {what}",
                    lambda: self.run(f"mode {mode}", note=f"Showing {what}", refresh=self.refresh_status))
        return {
            "display": [(ICON["display"], "Display test",
                         lambda: self.run("test", note="Display test running (any key stops it)"))],
            "screens": [(ICON["link"], "Apply to the board", self._screen_apply)],
            "countdown": [show("countdown", "countdown")],
            "countup": [show("countup", "count-up")],
            "messages": [show("messages", "messages")],
            "game": [show("pingpong", "ping-pong")],
            "settings": [(ICON["refresh"], "Reload everything", self.refresh_all)],
            "console": [(ICON["delete"], "Clear", lambda: self.con_out.delete("1.0", "end"))],
        }.get(key, [])

    def show_page(self, key: str) -> None:
        if self.page == key:
            return
        if self.page is not None:
            self.pages[self.page].grid_remove()
        self.page = key
        self.pages[key].grid(row=0, column=0, sticky="nsew")
        _k, title, icon, desc = next(p for p in PAGES if p[0] == key)
        for k, item in self.activity.items():
            item.set_active(k == key)
        self.tab_icon.configure(text=ICON[icon])
        self.tab_title.configure(text=title)
        self.crumb.configure(text=f"Beacon  \u203a  {title}      {desc}")
        for w in self.tab_actions.winfo_children():
            w.destroy()
        for glyph, text, cmd in self._page_actions(key):
            row = ctk.CTkFrame(self.tab_actions, fg_color="transparent")
            row.pack(side="left", padx=2)
            b = ctk.CTkButton(row, text=text, command=cmd, height=26, fg_color="transparent",
                              hover_color=C["hover"], text_color=C["text"], font=theme.font(12), width=10)
            ic = ctk.CTkLabel(row, text=glyph, font=theme.icon_font(13), text_color=C["accent"], width=16)
            ic.pack(side="left", padx=(6, 0))
            ic.bind("<Button-1>", lambda _e, c=cmd: c())
            b.pack(side="left", pady=5)
        if key == "console":
            self.after(50, self.con_in.focus_set)

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

        ctk.CTkButton(f, text="Display test (30 s)", width=150, height=44, fg_color=C["button2"], hover_color=C["button2_hover"],
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

        f = self._section(tab, "Refresh rate")
        self.refresh = ctk.CTkSlider(f, from_=20, to=680, number_of_steps=660, width=520,
                                     command=lambda v: self.refresh_lbl.configure(text=f"{int(v)} Hz"))
        self.refresh.grid(row=1, column=0, padx=12, pady=12)
        self.refresh.bind("<ButtonPress-1>", lambda _e: setattr(self, "_slider_dragging", True))
        self.refresh.bind("<ButtonRelease-1>", self._refresh_release)
        self.refresh_lbl = ctk.CTkLabel(f, text="- Hz", width=90)
        self.refresh_lbl.grid(row=1, column=1)
        ctk.CTkButton(f, text="Original", width=80, fg_color=C["button2"], hover_color=C["button2_hover"],
                      command=lambda: self._set_refresh("refresh 0", "original 2016 timing")
                      ).grid(row=1, column=2, padx=6)
        self.refresh_info = ctk.CTkLabel(f, text="Right: faster refresh, less flicker, a little dimmer at 100 %",
                                         anchor="w", text_color=C["muted"])
        self.refresh_info.grid(row=2, column=0, columnspan=3, sticky="w", padx=12, pady=(0, 10))

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

    def _refresh_release(self, _e) -> None:
        self._slider_dragging = False
        hz = int(self.refresh.get())
        self._set_refresh(f"refresh hz {hz}", f"{hz} Hz")

    def _show_refresh(self, kv: dict) -> None:
        try:
            hz = int(kv["refresh_hz"])
        except (KeyError, ValueError):
            return
        self.refresh.set(min(max(hz, 20), 680))
        unit = kv.get("unit", kv.get("refresh_unit", "?"))
        self.refresh_lbl.configure(text=f"{hz} Hz")
        text = f"{hz} Hz refresh, unit {unit}" + (" (original 2016 timing)" if unit == "0" else "")
        if "max_light" in kv:
            text += f", max light {kv['max_light']}"
        self.refresh_info.configure(text=text + "  (saved)")

    def _set_refresh(self, cmd: str, what: str) -> None:
        self.run(cmd, done=lambda reply: self._show_refresh(reply.kv() if reply else {}),
                 note=f"Refresh: {what}")

    # ------------------------------------------------------------ Game

    def _build_game(self, tab) -> None:
        f = self._section(tab, "Players")
        ctk.CTkLabel(f, text="Side 1").grid(row=1, column=0, padx=12, sticky="w")
        self.p1 = ctk.CTkOptionMenu(f, values=["-"], width=200)
        self.p1.grid(row=1, column=1, padx=6, pady=8)
        ctk.CTkLabel(f, text="Side 2").grid(row=1, column=2, padx=10)
        self.p2 = ctk.CTkOptionMenu(f, values=["-"], width=200)
        self.p2.grid(row=1, column=3, padx=6, pady=8)
        ctk.CTkButton(f, text="Show ping-pong", command=lambda: self.run("mode pingpong", refresh=self.refresh_status)
                      ).grid(row=2, column=0, columnspan=2, padx=6, pady=(4, 12), sticky="w")
        ctk.CTkButton(f, text="Select players", command=self._game_select).grid(row=2, column=2, padx=6, pady=(4, 12))
        ctk.CTkButton(f, text="Start game", fg_color=C["ok"], hover_color=C["ok_hover"],
                      command=lambda: self._game("game start", "Game started")).grid(row=2, column=3, padx=6, pady=(4, 12))

        f = self._section(tab, "Score")
        self.score_lbl = ctk.CTkLabel(f, text="0  :  0", font=ctk.CTkFont(size=64, weight="bold"))
        self.score_lbl.grid(row=1, column=0, columnspan=4, pady=8)
        for side, col in ((1, 0), (2, 2)):
            ctk.CTkButton(f, text=f"+1  side {side}", width=160, height=50, font=ctk.CTkFont(size=18),
                          command=lambda s=side: self._game(f"game point {s}")).grid(row=2, column=col, padx=8, pady=6)
            ctk.CTkButton(f, text="undo", width=80, fg_color=C["button2"], hover_color=C["button2_hover"],
                          command=lambda s=side: self._game(f"game undo {s}")).grid(row=2, column=col + 1, padx=8, pady=6)
        self.game_state = ctk.CTkLabel(f, text="", text_color=C["muted"])
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
        ctk.CTkLabel(f, text="Date").grid(row=1, column=0, padx=(12, 4), sticky="w")
        self.cd_date = ctk.CTkEntry(f, placeholder_text="YYYY-MM-DD", width=120)
        self.cd_date.grid(row=1, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="Time").grid(row=1, column=2, padx=(12, 4), sticky="w")
        self.cd_time = ctk.CTkEntry(f, placeholder_text="HH:MM[:SS]", width=100)
        self.cd_time.grid(row=1, column=3, padx=4)
        ctk.CTkButton(f, text="Set", width=80, command=self._cd_set).grid(row=1, column=4, padx=8)
        ctk.CTkLabel(f, text="or for").grid(row=2, column=0, padx=(12, 4), sticky="w")
        self.cd_in = ctk.CTkEntry(f, placeholder_text="1h30m, 2d, 45s", width=120)
        self.cd_in.grid(row=2, column=1, padx=4, pady=(4, 10))
        ctk.CTkButton(f, text="Start", width=80, command=lambda: self.cd_in.get() and self.run(
            f"cd in {self.cd_in.get().strip()}", note="Countdown started", refresh=self._load_timers)
                      ).grid(row=2, column=2, padx=8, pady=(4, 10))
        self.cd_info = ctk.CTkLabel(f, text="", text_color=C["muted"])
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
        ctk.CTkLabel(f, text="Scene").grid(row=1, column=0, padx=(12, 4), sticky="w")
        self.cd_scene = ctk.CTkOptionMenu(f, values=SCENES, width=130)
        self.cd_scene.grid(row=1, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="for").grid(row=1, column=2, padx=4)
        self.cd_scene_s = ctk.CTkEntry(f, width=70)
        self.cd_scene_s.insert(0, "30")
        self.cd_scene_s.grid(row=1, column=3, padx=4)
        self.cd_scene_inf = ctk.BooleanVar(value=False)
        ctk.CTkCheckBox(f, text="seconds  /  infinite", variable=self.cd_scene_inf).grid(row=1, column=4, padx=6)
        ctk.CTkLabel(f, text="then").grid(row=2, column=0, padx=(12, 4), sticky="w")
        self.cd_then = ctk.CTkOptionMenu(f, values=["hold", "screen"], width=130)
        self.cd_then.grid(row=2, column=1, padx=4, pady=6)
        ctk.CTkLabel(f, text="(hold = 00:00:00 + text, screen = 'Countdown end' layout)", text_color=C["muted"]
                     ).grid(row=2, column=2, columnspan=4, sticky="w", padx=4)
        ctk.CTkButton(f, text="Save", command=self._cd_after).grid(row=3, column=0, columnspan=2, padx=10, pady=(4, 12), sticky="w")
        ctk.CTkButton(f, text="Clear the end show", fg_color=C["button2"], hover_color=C["button2_hover"], command=lambda: self.run(
            "cd dismiss", note="End show cleared")).grid(row=3, column=2, columnspan=2, pady=(4, 12))
        self._icon_button(f, ICON["play"], "Try the scene now", self._scene_try, "ok").grid(
            row=1, column=5, padx=(20, 6), pady=6, sticky="w")
        ctk.CTkButton(f, text="Stop", width=70, fg_color=C["button2"], hover_color=C["button2_hover"],
                      command=lambda: self.run("scene stop", note="Scene stopped")).grid(row=1, column=6, padx=6)
        ctk.CTkLabel(f, text="Plays on the board now with the text at zero; the countdown is not touched. "
                             "Keyboard: Left / Right other scene, any key stops.",
                     text_color=C["faint"], font=theme.font(11)).grid(row=4, column=0, columnspan=7, padx=12,
                                                                        pady=(0, 10), sticky="w")

        f = self._section(tab, "")
        ctk.CTkButton(f, text="Show countdown", command=lambda: self.run("mode countdown", refresh=self.refresh_status)
                      ).grid(row=1, column=0, padx=10, pady=10)
        ctk.CTkButton(f, text="Remove countdown", fg_color=C["danger"], hover_color=C["danger_hover"], command=lambda: self.run(
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

    def _scene_try(self) -> None:
        scene = self.cd_scene.get()
        if scene == "none":
            return self.show_error(Exception("choose a scene first"))
        secs = self.cd_scene_s.get().strip()
        secs = "inf" if self.cd_scene_inf.get() else (secs if secs.isdigit() and int(secs) > 0 else "20")
        self.run(f"scene {scene} {secs}", note=f"Playing {scene} on the board ({secs} s)" if secs != "inf"
                 else f"Playing {scene} on the board until stopped")

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
        ctk.CTkButton(f, text="Stop", width=100, fg_color=C["button2"], hover_color=C["button2_hover"], command=lambda: self.run(
            "cu stop", refresh=self._load_timers)).grid(row=2, column=0, padx=10, pady=(4, 10))
        ctk.CTkButton(f, text="Resume", width=100, fg_color=C["button2"], hover_color=C["button2_hover"], command=lambda: self.run(
            "cu resume", refresh=self._load_timers)).grid(row=2, column=1, padx=4, pady=(4, 10))
        self.cu_info = ctk.CTkLabel(f, text="", text_color=C["muted"])
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
        ctk.CTkButton(f, text="Remove count-up", fg_color=C["danger"], hover_color=C["danger_hover"], command=lambda: self.run(
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

        left = ctk.CTkFrame(tab, fg_color=C["card"], border_width=1, border_color=C["border"])
        left.grid(row=0, column=0, sticky="nsew", padx=(14, 7), pady=(4, 14))
        ctk.CTkLabel(left, text="MESSAGES (PLAY ORDER)", font=theme.font(11, "bold"), text_color=C["muted"]).pack(
            anchor="w", padx=12, pady=(10, 2))
        self.msg_list = ctk.CTkScrollableFrame(left, fg_color=C["bg"])
        self.msg_list.pack(fill="both", expand=True, padx=6, pady=6)
        self.msg_count = ctk.CTkLabel(left, text="", text_color=C["muted"])
        self.msg_count.pack(anchor="w", padx=10)

        s = ctk.CTkFrame(left, fg_color="transparent")
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

        right = ctk.CTkFrame(tab, fg_color=C["card"], border_width=1, border_color=C["border"])
        right.grid(row=0, column=1, sticky="nsew", padx=(7, 14), pady=(4, 14))
        self.msg_editor_title = ctk.CTkLabel(right, text="NEW MESSAGE", font=theme.font(11, "bold"), text_color=C["muted"])
        self.msg_editor_title.pack(anchor="w", padx=12, pady=(10, 4))
        self.msg_l1 = ctk.CTkEntry(right, placeholder_text="Line 1 (up to 40 characters)")
        self.msg_l1.pack(fill="x", padx=10, pady=4)
        self.msg_l2 = ctk.CTkEntry(right, placeholder_text="Line 2 (optional)")
        self.msg_l2.pack(fill="x", padx=10, pady=4)
        self.msg_hint = ctk.CTkLabel(right, text="", text_color=C["muted"], justify="left")
        self.msg_hint.pack(anchor="w", padx=10)
        row = ctk.CTkFrame(right, fg_color="transparent")
        row.pack(fill="x", padx=10, pady=6)
        ctk.CTkLabel(row, text="Colour").pack(side="left")
        self.msg_colour = ctk.CTkOptionMenu(row, values=COLOURS, width=120, command=lambda _v: self._msg_preview())
        self.msg_colour.pack(side="left", padx=8)
        self.msg_save_btn = ctk.CTkButton(row, text="Add", width=90, command=self._msg_save)
        self.msg_save_btn.pack(side="left", padx=4)
        ctk.CTkButton(row, text="New", width=70, fg_color=C["button2"], hover_color=C["button2_hover"], command=self._msg_new).pack(side="left", padx=4)
        for e in (self.msg_l1, self.msg_l2):
            e.bind("<KeyRelease>", lambda _e: self._msg_preview())
        ctk.CTkLabel(right, text="Preview (Messages mode, as on the board):", text_color=C["muted"]).pack(anchor="w", padx=10, pady=(8, 0))
        self.msg_preview = ctk.CTkLabel(right, text="")
        self.msg_preview.pack(padx=10, pady=6)
        self._msg_preview()

    def _fill_messages(self) -> None:
        for w in self.msg_list.winfo_children():
            w.destroy()
        for m in self.messages:
            row = ctk.CTkFrame(self.msg_list, fg_color=C["card2"])
            row.pack(fill="x", pady=2)
            swatch = {"white": "#ffffff", "red": "#ff3333", "green": "#33ff33", "blue": "#3366ff",
                      "yellow": "#ffff33", "cyan": "#33ffff", "magenta": "#ff33ff"}.get(m.colour, "#ffffff")
            ctk.CTkLabel(row, text=f"{m.id:>2}", width=26).pack(side="left", padx=(6, 2))
            ctk.CTkLabel(row, text="●", text_color=swatch, width=16).pack(side="left")
            text = m.line1 + (f"  /  {m.line2}" if m.line2 else "")
            ctk.CTkLabel(row, text=text, anchor="w").pack(side="left", fill="x", expand=True, padx=6)
            ctk.CTkButton(row, text="Del", width=44, fg_color=C["danger"], hover_color=C["danger_hover"],
                          command=lambda i=m.id: self.run(f"msg del {i}", note=f"Message {i} deleted",
                                                          refresh=self._load_messages)).pack(side="right", padx=2)
            ctk.CTkButton(row, text="Show", width=50, fg_color=C["button2"], hover_color=C["button2_hover"],
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
        self.msg_editor_title.configure(text="NEW MESSAGE")
        self.msg_save_btn.configure(text="Add")
        for e in (self.msg_l1, self.msg_l2):
            e.delete(0, "end")
        self._msg_preview()

    def _msg_edit(self, m: Message) -> None:
        self.editing_id = m.id
        self.msg_editor_title.configure(text=f"MESSAGE {m.id}")
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
        top.pack(fill="x", padx=14, pady=(4, 2))
        ctk.CTkLabel(top, text="Screen").pack(side="left", padx=(0, 10))
        self.scr_name = ctk.CTkSegmentedButton(top, values=[SCREEN_TITLES[s] for s in SCREENS],
                                               command=lambda _v: self._fill_screen_form())
        self.scr_name.set(SCREEN_TITLES["clock"])
        self.scr_name.pack(side="left")

        f = self._section(tab, "Lines")
        heads = ["Line", "Shows", "Text colour", "Small", "Split", "Lower row", "Text colour", "Small", "Background"]
        for c, h in enumerate(heads):
            ctk.CTkLabel(f, text=h, text_color=C["muted"], font=theme.font(12)).grid(
                row=1, column=c, padx=6 if c else 12, pady=(0, 2), sticky="w")
        self.scr_rows = []
        half_items = [i for i in BAND_ITEMS if i not in ("big", "bigtext")]
        upd = lambda _v=None: self._update_screen_preview()
        for i in range(4):
            r = {}
            r["lbl"] = ctk.CTkLabel(f, text=f"{i + 1}", width=30, anchor="w")
            r["lbl"].grid(row=i + 2, column=0, padx=(12, 6), pady=3, sticky="w")
            r["top"] = ctk.CTkOptionMenu(f, values=BAND_ITEMS, width=110, command=upd)
            r["top"].grid(row=i + 2, column=1, padx=4, pady=3)
            r["fg0"] = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=95, command=upd)
            r["fg0"].grid(row=i + 2, column=2, padx=4, pady=3)
            r["small0"] = ctk.BooleanVar(value=False)
            ctk.CTkCheckBox(f, text="", variable=r["small0"], width=30, command=upd).grid(row=i + 2, column=3, padx=4)
            r["split"] = ctk.BooleanVar(value=False)
            ctk.CTkCheckBox(f, text="", variable=r["split"], width=30, command=self._split_changed).grid(row=i + 2, column=4, padx=4)
            r["bottom"] = ctk.CTkOptionMenu(f, values=half_items, width=110, command=upd)
            r["bottom"].grid(row=i + 2, column=5, padx=4, pady=3)
            r["fg1"] = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=95, command=upd)
            r["fg1"].grid(row=i + 2, column=6, padx=4, pady=3)
            r["small1"] = ctk.BooleanVar(value=False)
            r["small1_box"] = ctk.CTkCheckBox(f, text="", variable=r["small1"], width=30, command=upd)
            r["small1_box"].grid(row=i + 2, column=7, padx=4)
            r["bg"] = ctk.CTkOptionMenu(f, values=["black"] + COLOURS, width=95, command=upd)
            r["bg"].grid(row=i + 2, column=8, padx=4, pady=3)
            self.scr_rows.append(r)
        ctk.CTkLabel(f, text="Small = tiny font (32 characters) for text, time, ticker and msg rows.   "
                             "Text colour auto = the defaults below / a message's own colour.",
                     text_color=C["faint"], font=theme.font(11)).grid(row=6, column=0, columnspan=9, padx=12,
                                                                        pady=(2, 10), sticky="w")
        f = self._section(tab, "Messages and default colours")
        ctk.CTkLabel(f, text="Messages on this screen").grid(row=5, column=0, padx=12, pady=4, sticky="w")
        self.scr_msgs = ctk.CTkEntry(f, width=160, placeholder_text="all / none / 1 3 5")
        self.scr_msgs.grid(row=5, column=1, padx=6, pady=4, sticky="w")
        self.scr_msgs.bind("<KeyRelease>", lambda _e: self._update_screen_preview())
        ctk.CTkLabel(f, text="Default big digits colour").grid(row=6, column=0, padx=12, pady=4, sticky="w")
        self.scr_big = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_big.grid(row=6, column=1, padx=6, pady=4, sticky="w")
        ctk.CTkLabel(f, text="Default text colour").grid(row=7, column=0, padx=12, pady=4, sticky="w")
        self.scr_text = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_text.grid(row=7, column=1, padx=6, pady=4, sticky="w")
        ctk.CTkLabel(f, text="Hourglass colour").grid(row=8, column=0, padx=12, pady=4, sticky="w")
        self.scr_sand = ctk.CTkOptionMenu(f, values=["auto"] + COLOURS, width=120, command=lambda _v: self._update_screen_preview())
        self.scr_sand.grid(row=8, column=1, padx=6, pady=(4, 12), sticky="w")
        self._icon_button(f, ICON["link"], "Apply to the board", self._screen_apply).grid(
            row=5, column=2, rowspan=2, padx=24, sticky="w")

        f = self._section(tab, "Preview (as on the board)")
        self.scr_note = ctk.CTkLabel(f, text="", text_color=C["muted"], justify="left")
        self.scr_note.grid(row=1, column=0, sticky="w", padx=12)
        self.scr_preview = ctk.CTkLabel(f, text="")
        self.scr_preview.grid(row=2, column=0, sticky="w", padx=12, pady=(0, 12))

    def _screen_key(self) -> str:
        title = self.scr_name.get()
        return next(k for k, v in SCREEN_TITLES.items() if v == title)

    def _fill_screen_form(self) -> None:
        cfg = self.screens.get(self._screen_key())
        if not cfg:
            self._update_screen_preview()
            return
        for i, r in enumerate(self.scr_rows):
            parts = cfg["lines"][i].split("/")
            r["top"].set(parts[0])
            r["split"].set(len(parts) == 2)
            r["bottom"].set(parts[1] if len(parts) == 2 else "text")
            r["bg"].set(cfg["bg"][i])
            fg = cfg.get("fg", [["auto", "auto"]] * 4)[i]
            small = cfg.get("small", [[False, False]] * 4)[i]
            r["fg0"].set(fg[0])
            r["fg1"].set(fg[1])
            r["small0"].set(small[0])
            r["small1"].set(small[1])
        self.scr_msgs.delete(0, "end")
        self.scr_msgs.insert(0, cfg["msgs"].replace(",", " "))
        self.scr_big.set(cfg.get("big", "auto"))
        self.scr_text.set(cfg.get("text", "auto"))
        self.scr_sand.set(cfg.get("sand", "auto"))
        self._split_changed()

    def _split_changed(self) -> None:
        for r in self.scr_rows:
            state = "normal" if r["split"].get() else "disabled"
            for w in ("bottom", "fg1", "small1_box"):
                r[w].configure(state=state)
            if r["split"].get() and r["top"].get() in ("big", "bigtext"):
                r["top"].set("time" if r["top"].get() == "big" else "text")
        self._update_screen_preview()

    def _screen_layout(self) -> dict:
        lines, bgs, fgs, smalls = [], [], [], []
        for r in self.scr_rows:
            split = r["split"].get()
            lines.append(f"{r['top'].get()}/{r['bottom'].get()}" if split else r["top"].get())
            bgs.append(r["bg"].get())
            fgs.append([r["fg0"].get(), r["fg1"].get() if split else "auto"])
            smalls.append([r["small0"].get(), r["small1"].get() if split else False])
        return {"lines": lines, "bg": bgs, "fg": fgs, "small": smalls}

    def _screen_apply(self) -> None:
        s = self._screen_key()
        layout = self._screen_layout()
        cmds = [f"screen {s} {' '.join(layout['lines'])}"]
        cmds += [f"screen {s} {i + 1} bg {c}" for i, c in enumerate(layout["bg"])]
        cmds += [f"screen {s} {i + 1} colour {a} {b}" for i, (a, b) in enumerate(layout["fg"])]
        cmds += [f"screen {s} {i + 1} small {'on' if a else 'off'} {'on' if b else 'off'}"
                 for i, (a, b) in enumerate(layout["small"])]
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
        # tiny items: the countdown (count-up screen: elapsed) and the date/time now
        src = self.cu.get("elapsed", "") if screen == "countup" else self.cd.get("remaining", "")
        tm = re.match(r"(\d+)d (\d\d:\d\d:\d\d)", src)
        timer = (f"{int(tm.group(1))}D {tm.group(2)}" if int(tm.group(1)) else tm.group(2)) if tm else "--:--:--"
        nowdt = dt.datetime.now()
        now = f"{nowdt.strftime('%a').upper()} {nowdt.day} {nowdt.strftime('%b').upper()} {nowdt:%H:%M:%S}"
        return PreviewData(big=big, text=text, messages=[(m.line1, m.line2, m.colour) for m in msgs],
                           timer=timer, now=now, year=nowdt.year,
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

        f = self._section(tab, "Board LEDs")
        ctk.CTkLabel(f, text="Brightness").grid(row=1, column=0, padx=12, pady=4, sticky="w")
        self.led_bright = ctk.CTkSlider(f, from_=0, to=100, number_of_steps=20, width=300,
                                        command=lambda v: self.led_bright_lbl.configure(text=f"{int(v)} %"))
        self.led_bright.grid(row=1, column=1, padx=6, pady=4, sticky="w")
        self.led_bright.bind("<ButtonRelease-1>", lambda _e: self.run(
            f"led bright {int(self.led_bright.get())}", note=f"Board LEDs {int(self.led_bright.get())} %"))
        self.led_bright_lbl = ctk.CTkLabel(f, text="- %", width=50)
        self.led_bright_lbl.grid(row=1, column=2, padx=6, sticky="w")
        ctk.CTkLabel(f, text="Heartbeat (green)").grid(row=2, column=0, padx=12, pady=4, sticky="w")
        self.led_hb = ctk.CTkSegmentedButton(f, values=["Fade", "Blink"], command=lambda v: self.run(
            f"led heartbeat {v.lower()}", note=f"Heartbeat: {v.lower()}"))
        self.led_hb.grid(row=2, column=1, padx=6, pady=4, sticky="w")
        ctk.CTkButton(f, text="Test LEDs", width=100, fg_color=C["button2"], hover_color=C["button2_hover"],
                      command=lambda: self.run("led test", note="LED test: green, orange, red, blue")
                      ).grid(row=2, column=2, padx=6, pady=4, sticky="w")
        self.led_info = ctk.CTkLabel(f, text="Green heartbeat   Blue Bluetooth   Orange clock sync / keys   "
                                             "Red error codes (1 EEPROM, 2 Bluetooth, 3 RTC trim)",
                                     text_color=C["faint"], font=theme.font(11))
        self.led_info.grid(row=3, column=0, columnspan=4, padx=12, pady=(2, 10), sticky="w")

        f = self._section(tab, "Bluetooth (change while connected over USB serial)")
        ctk.CTkLabel(f, text="Name").grid(row=1, column=0, padx=12, sticky="w")
        self.ble_name = ctk.CTkEntry(f, width=160)
        self.ble_name.grid(row=1, column=1, padx=4, pady=4)
        ctk.CTkButton(f, text="Set", width=60, command=lambda: self.run(
            f"ble name {self.ble_name.get().strip()}", note="Bluetooth name set")).grid(row=1, column=2, padx=6)
        ctk.CTkLabel(f, text="PIN").grid(row=2, column=0, padx=12, pady=(4, 10), sticky="w")
        self.ble_pin = ctk.CTkEntry(f, width=160, placeholder_text="6 digits or off")
        self.ble_pin.grid(row=2, column=1, padx=4, pady=(4, 10))
        ctk.CTkButton(f, text="Set", width=60, command=lambda: self.run(
            f"ble pin {self.ble_pin.get().strip()}", note="Bluetooth PIN set")).grid(row=2, column=2, padx=6, pady=(4, 10))

        f = self._section(tab, "Board")
        ctk.CTkButton(f, text="Reload everything", command=self.refresh_all).grid(row=1, column=0, padx=10, pady=(4, 10))
        ctk.CTkButton(f, text="Restart the board", fg_color=C["danger"], hover_color=C["danger_hover"],
                      command=self._reboot).grid(row=1, column=1, padx=10, pady=(4, 10))
        self.board_info = ctk.CTkLabel(f, text="", justify="left", text_color=C["muted"])
        self.board_info.grid(row=2, column=0, columnspan=3, padx=10, pady=(0, 10), sticky="w")

    def _fill_settings(self) -> None:
        self.board_info.configure(text="   ".join(f"{k}: {v}" for k, v in self.info.items()))

    def _fill_leds(self, kv: dict) -> None:
        try:
            self.led_bright.set(int(kv.get("led_bright", "15")))
            self.led_bright_lbl.configure(text=f"{kv.get('led_bright', '-')} %")
        except ValueError:
            pass
        self.led_hb.set("Blink" if kv.get("heartbeat") == "blink" else "Fade")
        errors = kv.get("errors", "none")
        if errors != "none":
            self.led_info.configure(text=f"Errors: {errors}", text_color=C["error"])

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
        self.con_out = ctk.CTkTextbox(tab, font=ctk.CTkFont(family=theme.mono_family(), size=13))
        self.con_out.pack(fill="both", expand=True, padx=14, pady=(4, 6))
        row = ctk.CTkFrame(tab, fg_color="transparent")
        row.pack(fill="x", padx=14, pady=(0, 14))
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
