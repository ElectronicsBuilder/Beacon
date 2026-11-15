"""VS Code style look for the Beacon control panel: colours, fonts, icon glyphs, app icon.

The CustomTkinter theme is the built-in "blue" theme with the colours of VS Code's
"Dark Modern" theme patched in (so it keeps working with newer CustomTkinter
versions). Icons are glyphs of the Windows icon font (Segoe Fluent Icons on
Windows 11, Segoe MDL2 Assets on Windows 10) - the same style VS Code uses.
"""
from __future__ import annotations

import ctypes
import json
import os
import sys
import tkinter.font as tkfont
from pathlib import Path

import customtkinter as ctk
from PIL import Image, ImageDraw

ASSETS = Path(__file__).with_name("assets")
ICON_ICO = ASSETS / "beacon.ico"
ICON_PNG = ASSETS / "beacon.png"
APP_ID = "ElectronicsBuilder.Beacon.ControlPanel"

# ---------------------------------------------------------------- colours (Dark Modern)

C = {
    "bg": "#1f1f1f",          # editor
    "panel": "#181818",       # side bar, activity bar, title strips
    "card": "#252526",        # sections
    "card2": "#2b2b2b",       # rows inside sections
    "border": "#2b2b2b",
    "input": "#313131",
    "input_border": "#3c3c3c",
    "hover": "#2a2d2e",
    "select": "#04395e",
    "accent": "#0078d4",      # focus, active tab line
    "button": "#0e639c",
    "button_hover": "#1177bb",
    "button2": "#313131",     # secondary button
    "button2_hover": "#3c3c3c",
    "danger": "#a1260d",
    "danger_hover": "#c72e0f",
    "ok": "#388a34",
    "ok_hover": "#45a240",
    "text": "#cccccc",
    "text_bright": "#ffffff",
    "muted": "#9d9d9d",
    "faint": "#6e7681",
    "error": "#f48771",
    "warn": "#cca700",
    "status_on": "#0078d4",   # status bar: connected
    "status_off": "#3a3d41",  # status bar: not connected
}

SWATCH = {"white": "#ffffff", "red": "#ff3333", "green": "#33ff33", "blue": "#3366ff",
          "yellow": "#ffff33", "cyan": "#33ffff", "magenta": "#ff33ff", "black": "#000000", "auto": "#9d9d9d"}

# ---------------------------------------------------------------- icon glyphs

ICON = {
    "display": "", "screens": "", "countdown": "", "countup": "",
    "messages": "", "game": "", "settings": "", "console": "",
    "bluetooth": "", "usb": "", "refresh": "", "clock": "", "link": "",
    "play": "", "delete": "", "edit": "", "add": "", "power": "",
    "keyboard": "", "board": "",
}

_icon_family: str | None = None


def icon_family() -> str:
    """Segoe Fluent Icons (Windows 11) or Segoe MDL2 Assets (Windows 10)."""
    global _icon_family
    if _icon_family is None:
        families = set(tkfont.families())
        _icon_family = next((f for f in ("Segoe Fluent Icons", "Segoe MDL2 Assets") if f in families), "Segoe UI Symbol")
    return _icon_family


def ui_family() -> str:
    return "Segoe UI" if sys.platform == "win32" else "Helvetica"


def mono_family() -> str:
    families = set(tkfont.families())
    return next((f for f in ("Cascadia Code", "Cascadia Mono", "Consolas") if f in families), "Courier")


def font(size: int = 13, weight: str = "normal") -> ctk.CTkFont:
    return ctk.CTkFont(family=ui_family(), size=size, weight=weight)


def icon_font(size: int = 16) -> ctk.CTkFont:
    return ctk.CTkFont(family=icon_family(), size=size)


# ---------------------------------------------------------------- CustomTkinter theme

def _both(c: str) -> list[str]:
    return [c, c]


def apply_theme() -> None:
    """Write the patched theme next to this file and make it the default."""
    base = Path(ctk.__file__).parent / "assets" / "themes" / "blue.json"
    t = json.loads(base.read_text(encoding="utf-8"))
    t["CTk"]["fg_color"] = _both(C["bg"])
    t["CTkToplevel"]["fg_color"] = _both(C["bg"])
    t["CTkFrame"].update(corner_radius=6, fg_color=_both(C["card"]), top_fg_color=_both(C["card2"]),
                         border_color=_both(C["border"]))
    t["CTkButton"].update(corner_radius=4, fg_color=_both(C["button"]), hover_color=_both(C["button_hover"]),
                          text_color=_both(C["text_bright"]), text_color_disabled=_both(C["faint"]))
    t["CTkLabel"].update(text_color=_both(C["text"]))
    t["CTkEntry"].update(corner_radius=4, border_width=1, fg_color=_both(C["input"]),
                         border_color=_both(C["input_border"]), text_color=_both(C["text"]),
                         placeholder_text_color=_both(C["faint"]))
    t["CTkCheckBox"].update(corner_radius=4, border_width=2, fg_color=_both(C["button"]),
                            border_color=_both("#6e6e6e"), hover_color=_both(C["button_hover"]),
                            checkmark_color=_both(C["text_bright"]), text_color=_both(C["text"]))
    t["CTkSwitch"].update(fg_color=_both(C["input_border"]), progress_color=_both(C["button"]),
                          button_color=_both(C["text"]), button_hover_color=_both(C["text_bright"]))
    t["CTkSlider"].update(fg_color=_both(C["input_border"]), progress_color=_both(C["accent"]),
                          button_color=_both(C["text"]), button_hover_color=_both(C["text_bright"]))
    t["CTkProgressBar"].update(fg_color=_both(C["input_border"]), progress_color=_both(C["accent"]))
    t["CTkOptionMenu"].update(corner_radius=4, fg_color=_both(C["input"]), button_color=_both(C["input"]),
                              button_hover_color=_both(C["button2_hover"]), text_color=_both(C["text"]))
    t["CTkComboBox"].update(corner_radius=4, border_width=1, fg_color=_both(C["input"]),
                            border_color=_both(C["input_border"]), button_color=_both(C["input_border"]),
                            button_hover_color=_both("#4c4c4c"), text_color=_both(C["text"]))
    t["CTkScrollbar"].update(fg_color="transparent", button_color=_both("#424242"),
                             button_hover_color=_both("#4f4f4f"))
    t["CTkSegmentedButton"].update(corner_radius=4, fg_color=_both(C["input"]), selected_color=_both(C["button"]),
                                   selected_hover_color=_both(C["button_hover"]),
                                   unselected_color=_both(C["input"]), unselected_hover_color=_both(C["button2_hover"]),
                                   text_color=_both(C["text_bright"]))
    t["CTkTextbox"].update(corner_radius=4, fg_color=_both(C["bg"]), border_color=_both(C["border"]),
                           text_color=_both(C["text"]), scrollbar_button_color=_both("#424242"),
                           scrollbar_button_hover_color=_both("#4f4f4f"))
    t["CTkScrollableFrame"].update(label_fg_color=_both(C["card2"]))
    t["DropdownMenu"].update(fg_color=_both(C["card"]), hover_color=_both(C["select"]), text_color=_both(C["text"]))
    for os_name in t["CTkFont"]:
        t["CTkFont"][os_name]["family"] = "Segoe UI" if os_name == "Windows" else t["CTkFont"][os_name]["family"]
    out = ASSETS / "vscode_dark.json"
    ASSETS.mkdir(exist_ok=True)
    out.write_text(json.dumps(t, indent=1), encoding="utf-8")
    ctk.set_appearance_mode("dark")
    ctk.set_default_color_theme(str(out))


# ---------------------------------------------------------------- app icon

def _draw_icon(size: int) -> Image.Image:
    """Beacon: a light with signal arcs on a dark rounded square."""
    s = 4 * size                                              # draw big, scale down (smooth)
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((s * 0.04, s * 0.04, s * 0.96, s * 0.96), radius=s // 5, fill=(22, 24, 30, 255))
    cx = cy = s * 0.5
    for rad, alpha in ((0.17, 255), (0.30, 200)):
        r = s * rad
        for start in (200, 290, 20, 110):                     # four arcs around the light
            d.arc((cx - r, cy - r, cx + r, cy + r), start, start + 50, fill=(255, 190, 40, alpha),
                  width=int(s * 0.055))
    r = s * 0.075
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 236, 170, 255))
    return img.resize((size, size), Image.LANCZOS)


def ensure_icon() -> Path:
    """Create assets/beacon.ico (and .png) once."""
    if not ICON_ICO.exists() or not ICON_PNG.exists():
        ASSETS.mkdir(exist_ok=True)
        big = _draw_icon(256)
        big.save(ICON_PNG)
        big.save(ICON_ICO, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    return ICON_ICO


def set_app_id() -> None:
    """Windows: own taskbar button and icon instead of python's (before the window exists)."""
    if sys.platform == "win32":
        try:
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(APP_ID)
        except (AttributeError, OSError):
            pass


def set_window_icon(win) -> None:
    """Title bar + taskbar icon. CustomTkinter sets its own icon ~200 ms after start, so
    set ours after that as well."""
    ico = ensure_icon()

    def apply():
        try:
            if sys.platform == "win32":
                win.iconbitmap(default=str(ico))
            else:
                from PIL import ImageTk
                win._icon_img = ImageTk.PhotoImage(Image.open(ICON_PNG))
                win.iconphoto(True, win._icon_img)
        except Exception:  # noqa: BLE001 - an icon is never worth a crash
            pass
    apply()
    win.after(300, apply)


if __name__ == "__main__":                          # python -m scoreboard.theme: redraw the icon
    ICON_ICO.unlink(missing_ok=True)
    ensure_icon()
