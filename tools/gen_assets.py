#!/usr/bin/env python3
"""Глифы 5x7 и координаты циферблата. Запуск из корня репозитория."""

import math
from pathlib import Path

GLYPHS = {
    " ": ["00000"] * 7,
    "+": ["00100", "00100", "11111", "00100", "00100", "00000", "00000"],
    "-": ["00000", "00000", "11111", "00000", "00000", "00000", "00000"],
    ".": ["00000", "00000", "00000", "00000", "00000", "01100", "01100"],
    "/": ["00001", "00010", "00010", "00100", "01000", "01000", "10000"],
    "%": ["11001", "11010", "00010", "00100", "01000", "01011", "10011"],
    ":": ["00000", "01100", "01100", "00000", "01100", "01100", "00000"],
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["01110", "10001", "00001", "00110", "00001", "10001", "01110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "B": ["11110", "10001", "10001", "11110", "10001", "10001", "11110"],
    "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01111"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    "J": ["00111", "00010", "00010", "00010", "00010", "10010", "01100"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "Q": ["01110", "10001", "10001", "10001", "10101", "10010", "01101"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    "W": ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    "Z": ["11111", "00001", "00010", "00100", "01000", "10000", "11111"],
}

for ch, rows in GLYPHS.items():
    if len(rows) != 7 or any(len(r) != 5 or set(r) - set("01") for r in rows):
        raise SystemExit("bad glyph %r" % ch)


def row_byte(bits: str) -> int:
    value = 0
    for col, bit in enumerate(bits):
        if bit == "1":
            value |= 0x10 >> col
    return value


def measure(text: str, scale: int) -> int:
    n = len(text)
    if n == 0:
        return 0
    return n * 5 * scale + (n - 1) * scale


def circle(n: int, radius: int):
    xs, ys = [], []
    for i in range(n):
        theta = math.radians(i * 360.0 / n)
        xs.append(int(round(math.sin(theta) * radius)))
        ys.append(int(round(-math.cos(theta) * radius)))
    return xs, ys


def dist_to_rect(px, py, x, y, w, h) -> float:
    cx = min(max(px, x), x + w - 1)
    cy = min(max(py, y), y + h - 1)
    return math.hypot(px - cx, py - cy)


CX = CY = 120
R_SEC = 98
R_HOUR = 110
SEC_DOT = 3
HOUR_DOT = 2

TIME_SCALE = 6
TIME_CW = 5 * TIME_SCALE
TIME_GAP = 4
TIME_COLON = 12
TIME_W = TIME_CW * 4 + TIME_COLON + TIME_GAP * 4
TIME_X = (240 - TIME_W) // 2
TIME_Y = 90
TIME_H = 7 * TIME_SCALE

DATE_SCALE = 2
DATE_TEXT = "TUE 10 NOV"
DATE_W = measure(DATE_TEXT, DATE_SCALE)
DATE_X = (240 - DATE_W) // 2
DATE_Y = 148
DATE_H = 7 * DATE_SCALE

BAT_W = 30
BAT_H = 14
BAT_X = (240 - BAT_W) // 2
BAT_Y = 176

HINT_TEXT = "B MENU"
HINT_W = measure(HINT_TEXT, 1)
HINT_X = (240 - HINT_W) // 2
HINT_Y = 200

MARK_TEXT = "AL T"
MARK_SCALE = 2
MARK_W = measure(MARK_TEXT, MARK_SCALE)
MARK_X = 64
MARK_Y = 62
MARK_H = 7 * MARK_SCALE

AM_W = measure("PM", MARK_SCALE)
AM_X = 240 - 64 - AM_W
AM_Y = MARK_Y

WIDGETS = [
    ("time", TIME_X, TIME_Y, TIME_W, TIME_H),
    ("date", DATE_X, DATE_Y, DATE_W, DATE_H),
    ("bat", BAT_X, BAT_Y, BAT_W, BAT_H),
    ("hint", HINT_X, HINT_Y, HINT_W, 7),
    ("mark", MARK_X, MARK_Y, MARK_W, MARK_H),
    ("ampm", AM_X, AM_Y, AM_W, MARK_H),
]

sec_x, sec_y = circle(60, R_SEC)
hour_x, hour_y = circle(12, R_HOUR)


def check():
    worst = 999.0
    for name, x, y, w, h in WIDGETS:
        for dx, dy in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)):
            d = math.hypot((x + dx) - CX, (y + dy) - CY)
            if d > 114:
                raise SystemExit("%s corner outside bezel: %.1f" % (name, d))
        for i, (dx, dy) in enumerate(zip(sec_x, sec_y)):
            gap = dist_to_rect(CX + dx, CY + dy, x, y, w, h) - SEC_DOT
            worst = min(worst, gap)
            if gap < 2:
                raise SystemExit("%s hits second %d gap %.2f" % (name, i, gap))
        for i, (dx, dy) in enumerate(zip(hour_x, hour_y)):
            gap = dist_to_rect(CX + dx, CY + dy, x, y, w, h) - HOUR_DOT
            if gap < 2:
                raise SystemExit("%s hits hour %d gap %.2f" % (name, i, gap))
    for i in range(12):
        # Та же минута: часовая точка и секундная на одном луче.
        gap = math.hypot(hour_x[i] - sec_x[i * 5], hour_y[i] - sec_y[i * 5]) - HOUR_DOT - SEC_DOT
        if gap < 2:
            raise SystemExit("hour/second overlap %d" % i)
    print("layout ok, min second gap %.2f" % worst)
    print("time", TIME_X, TIME_Y, TIME_W, TIME_H)
    print("date", DATE_X, DATE_Y, DATE_W)
    print("mark", MARK_X, MARK_Y, "am", AM_X)


def emit(path: Path):
    lines = []
    a = lines.append
    a("/* Сгенерировано tools/gen_assets.py. Не правьте числа вручную:")
    a("   координаты и глифы должны совпадать с проверкой пересечений. */")
    a("#ifndef ASSETS_H")
    a("#define ASSETS_H")
    a("#include <stdint.h>")
    a("#define UI_CX 120")
    a("#define UI_CY 120")
    a("#define UI_R_SEC %d" % R_SEC)
    a("#define UI_R_HOUR %d" % R_HOUR)
    a("#define UI_SEC_DOT %d" % SEC_DOT)
    a("#define UI_HOUR_DOT %d" % HOUR_DOT)
    a("#define UI_TIME_SCALE %d" % TIME_SCALE)
    a("#define UI_TIME_CW %d" % TIME_CW)
    a("#define UI_TIME_GAP %d" % TIME_GAP)
    a("#define UI_TIME_COLON %d" % TIME_COLON)
    a("#define UI_TIME_X %d" % TIME_X)
    a("#define UI_TIME_Y %d" % TIME_Y)
    a("#define UI_DATE_SCALE %d" % DATE_SCALE)
    a("#define UI_DATE_X %d" % DATE_X)
    a("#define UI_DATE_Y %d" % DATE_Y)
    a("#define UI_BAT_X %d" % BAT_X)
    a("#define UI_BAT_Y %d" % BAT_Y)
    a("#define UI_BAT_W %d" % BAT_W)
    a("#define UI_BAT_H %d" % BAT_H)
    a("#define UI_HINT_X %d" % HINT_X)
    a("#define UI_HINT_Y %d" % HINT_Y)
    a("#define UI_MARK_X %d" % MARK_X)
    a("#define UI_MARK_Y %d" % MARK_Y)
    a("#define UI_MARK_W %d" % MARK_W)
    a("#define UI_MARK_H %d" % MARK_H)
    a("#define UI_AM_X %d" % AM_X)
    a("#define UI_AM_Y %d" % AM_Y)
    a("#define UI_AM_W %d" % AM_W)
    a("#define UI_TIME_W %d" % TIME_W)
    a("#define UI_TIME_H %d" % TIME_H)
    a("/* Смещение от центра. Индекс 0 — 12 часов, дальше по часовой. */")
    def arr(name, values):
        body = ", ".join("%4d" % v for v in values)
        a("static const int8_t %s[%d] = {%s};" % (name, len(values), body))
    arr("dial_sec_dx", sec_x)
    arr("dial_sec_dy", sec_y)
    arr("dial_hour_dx", hour_x)
    arr("dial_hour_dy", hour_y)
    a("/* Глиф: 7 строк, бит 4 — левый столбец. Индекс = символ - 32, до 'Z'. */")
    a("static const uint8_t font5x7[59][7] = {")
    for code in range(32, 91):
        ch = chr(code)
        rows = GLYPHS.get(ch, ["00000"] * 7)
        nums = ", ".join("0x%02X" % row_byte(r) for r in rows)
        a("    {%s}, /* %s */" % (nums, ch if ch != "\\" else "\\\\"))
    a("};")
    a("#endif")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    check()
    out = Path(__file__).resolve().parents[1] / "src" / "assets.h"
    emit(out)
    print("wrote", out)
