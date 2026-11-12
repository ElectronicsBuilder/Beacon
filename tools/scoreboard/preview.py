"""Pixel-exact preview of the LED matrix (2 x 64x64 HUB75 panels, 128 x 64 LEDs).

Follows the firmware's layout rules (Modes/ui_bands.c, layout.h):
  - 4 lines (bands) of 16 rows; with 32 driven rows only lines 1 and 3 light
    (image rows 0-15 and 16-31), lines 2 and 4 stay dark;
  - big = 8x8 font at double size (16 px), time/text/ticker = 8 px rows,
    a full-height text line is centred (y + 4), a split line has two rows;
  - text of up to 16 characters is centred, longer text scrolls (the preview
    shows its start, marked with a small arrow);
  - this board's fault: lines 1-2 have blue stuck on and are drawn in white.
"""
from __future__ import annotations

from dataclasses import dataclass, field

from PIL import Image, ImageDraw

from .font8x8 import FIRST, FONT

WIDTH = 128
PHYS_ROWS = 64
CHARS = WIDTH // 8                  # 16 characters per 8 px row

RGB = {
    "white": (255, 255, 255), "red": (255, 0, 0), "green": (0, 255, 0), "blue": (0, 0, 255),
    "yellow": (255, 255, 0), "cyan": (0, 255, 255), "magenta": (255, 0, 255), "black": (0, 0, 0),
}
BG_LEVEL = 0.35                     # backgrounds are dimmed (MATRIX_BG_LEVEL 0x60, gamma)


HG_HW = (4, 4, 4, 3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 4, 4, 4)   # hourglass outline (ui_bands.c)
HG_W = 9


def draw_hourglass(m: "Matrix", x0: int, y0: int, top: int, bg: tuple, white_glass: bool) -> None:
    """Same drawing as the firmware: top = seconds of sand left in the top bulb."""
    cells = sum(2 * HG_HW[r] - 1 for r in range(1, 7))
    in_top = (top * cells + 30) // 60
    in_bottom = cells - in_top
    g = 0xC0 if white_glass else 0x70
    for r in range(16):
        for c in range(HG_W):
            d = abs(c - HG_W // 2)
            if r in (0, 15) or d == HG_HW[r]:
                m.set(x0 + c, y0 + r, (g, g, g))
            else:
                m.set(x0 + c, y0 + r, bg)
    for rows, left in ((range(6, 0, -1), in_top), (range(14, 8, -1), in_bottom)):
        for r in rows:
            for d in range(HG_HW[r]):
                for side in (-1, 1):
                    if left <= 0 or (d == 0 and side > 0):
                        continue
                    m.set(x0 + HG_W // 2 + side * d, y0 + r, (255, 190, 0))
                    left -= 1
    if in_top > 0 and top < 60:
        m.set(x0 + HG_W // 2, y0 + 7 + top % 2, (255, 190, 0))


@dataclass
class PreviewData:
    big: str = "12:34:56"           # big / time items
    big_red: bool = False           # countdown at zero
    text: str = "SUN 2026-11-09"    # text / ticker items
    messages: list = field(default_factory=list)   # [(line1, line2, colour)]
    seconds: int = 30               # hourglass: sand in the top bulb (big digits with days)
    big_colour: str = "auto"
    text_colour: str = "auto"


class Matrix:
    """The LED image as the firmware sees it (128 x driven rows)."""

    def __init__(self, rows: int = 32) -> None:
        self.rows = rows
        self.px = [[(0, 0, 0)] * WIDTH for _ in range(rows)]

    def set(self, x: int, y: int, c: tuple) -> None:
        if 0 <= x < WIDTH and 0 <= y < self.rows:
            self.px[y][x] = c

    def fill(self, y: int, h: int, c: tuple) -> None:
        for yy in range(y, y + h):
            for x in range(WIDTH):
                self.set(x, yy, c)

    def text(self, s: str, x: int, y: int, fg: tuple, bg: tuple | None = None, scale: int = 1,
             advance: int | None = None) -> None:
        for ch in s:
            code = ord(ch) - FIRST
            rows = FONT[code] if 0 <= code < len(FONT) else FONT[0]
            for r, bits in enumerate(rows):
                for c in range(8):
                    on = bits & (0x80 >> c)
                    if on or bg is not None:
                        for dy in range(scale):
                            for dx in range(scale):
                                self.set(x + c * scale + dx, y + r * scale + dy, fg if on else bg)
            x += advance or 8 * scale

    def line(self, s: str, y: int, fg: tuple, bg: tuple) -> None:
        """One 8 px row: centred, or the start of a scrolling text."""
        if len(s) <= CHARS:
            self.text(s, (WIDTH - len(s) * 8) // 2, y, fg, bg)
        else:
            self.text(s[:CHARS], 0, y, fg, bg)
            for i in range(3):                       # scroll marker at the right edge
                self.set(WIDTH - 1 - i, y + 7, (255, 160, 0))

    # ------------------------------------------------------------ physical panels

    def physical(self, top_fault: bool) -> list[list[tuple | None]]:
        """64 panel rows; None = row not driven (dark)."""
        out: list[list[tuple | None]] = []
        for prow in range(PHYS_ROWS):
            if self.rows == 64:
                y = prow
            else:
                band = prow // 16
                y = {0: prow, 2: prow - 16}.get(band, -1)
            if y < 0:
                out.append(None)
                continue
            row = list(self.px[y])
            if top_fault and prow < 32:              # blue input stuck on
                row = [(r, g, 255) for (r, g, b) in row]
            out.append(row)
        return out

    def image(self, scale: int = 6, top_fault: bool = True) -> Image.Image:
        img = Image.new("RGB", (WIDTH * scale, PHYS_ROWS * scale), (8, 8, 10))
        d = ImageDraw.Draw(img)
        r = max(1, scale // 2 - 1)
        for prow, row in enumerate(self.physical(top_fault)):
            cy = prow * scale + scale // 2
            for x in range(WIDTH):
                cx = x * scale + scale // 2
                if row is None:
                    c = (18, 18, 20)                 # not driven: LED stays off
                else:
                    p = row[x]
                    c = p if p != (0, 0, 0) else (34, 34, 38)
                d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=c)
        return img


def _band_y(band: int, rows: int) -> int:
    if rows == 64:
        return band * 16
    return {0: 0, 2: 16}.get(band, -1)


def _dim(colour: str) -> tuple:
    r, g, b = RGB.get(colour, (0, 0, 0))
    return (int(r * BG_LEVEL), int(g * BG_LEVEL), int(b * BG_LEVEL))


def render_screen(layout: dict, data: PreviewData, rows: int = 32, top_fault: bool = True,
                  msg_offset: int = 0) -> Matrix:
    """layout: {'lines': ['ticker', 'time/text', ...] x4, 'bg': [colour x4]}"""
    m = Matrix(rows)
    msgs = list(data.messages)
    msg_i = msg_offset

    def next_msg():
        nonlocal msg_i
        if not msgs:
            return None
        msg = msgs[msg_i % len(msgs)]
        msg_i += 1
        return msg

    for band in range(4):
        y0 = _band_y(band, rows)
        if y0 < 0:
            continue
        top_white = top_fault and band < 2
        bg = _dim(layout["bg"][band]) if layout.get("bg") else (0, 0, 0)
        m.fill(y0, 16, bg)
        parts = layout["lines"][band].split("/")
        slots = [(parts[0], y0, True)] if len(parts) == 1 else [(parts[0], y0, False), (parts[1], y0 + 8, False)]
        for kind, y, full in slots:
            col = lambda name: RGB["white"] if top_white else RGB.get(name, RGB["white"])
            big_rgb = RGB["red"] if data.big_red else RGB.get(data.big_colour, RGB["white"])
            if kind == "big":
                n = len(data.big)
                hourglass = "D" in data.big and n <= 8              # "49D05:12" + hourglass
                slots = n + (1 if hourglass else 0)
                adv = 16 if not hourglass and slots * 16 <= WIDTH else 14
                x0 = (WIDTH - slots * adv + (adv - 16)) // 2
                m.text(data.big, x0, y, big_rgb, bg, 2, advance=adv)
                if hourglass:
                    draw_hourglass(m, x0 + n * adv + (adv - HG_W) // 2 + 1, y, data.seconds, bg, top_white)
            elif kind == "time":
                c = RGB["red"] if data.big_red else (big_rgb if data.big_colour != "auto" else col("white"))
                m.line(data.big, y + 4 if full else y, c, bg)
            elif kind in ("text", "ticker"):
                c = RGB.get(data.text_colour) if data.text_colour != "auto" else col("green")
                m.line(data.text, y + 4 if full else y, c, bg)
            elif kind == "msg":
                msg = next_msg()
                if msg is None:
                    continue
                l1, l2, colour = msg
                if full and l2:
                    m.line(l1, y, col(colour), bg)
                    m.line(l2, y + 8, col(colour), bg)
                else:
                    m.line(f"{l1}  {l2}" if l2 else l1, y + 4 if full else y, col(colour), bg)
    return m


def render_message(line1: str, line2: str, colour: str, rows: int = 32, top_fault: bool = True) -> Matrix:
    """A message as the Messages mode (F5) shows it."""
    m = Matrix(rows)
    c = RGB.get(colour, RGB["white"])
    if rows == 64:
        y1, y2, single = 20, 36, 28
    else:
        y1, y2, single = 4, 20, 20
    if line2:
        m.line(line1, y1, RGB["white"] if top_fault and rows == 32 else c, None)
        m.line(line2, y2, c, None)
    else:
        m.line(line1, single, c, None)
    return m
