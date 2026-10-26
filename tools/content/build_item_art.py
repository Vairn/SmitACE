#!/usr/bin/env python3
"""Pixel-art item sprites in the factory wallset palette.

Floor sprites sit on the floor strip (wide, short). The pack icon is a
smaller copy. Colour index 0 is transparent. Opaque pixels are stored
as 32+index so they land in the wallset bank (screen colours 32-47).
"""

import os
from collections import Counter

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "data", "items")

W, H = 32, 16
# Wallset indices. 0 stays empty so the mask can cut it out.
CLR = {
    "outline": 2,
    "shadow": 4,
    "gray": 7,
    "light": 12,
    "white": 15,
    "rust": 6,
    "red": 3,
    "gold": 13,
    "blue": 8,
    "navy": 5,
    "darkred": 1,
}


def blank():
    return [[0] * W for _ in range(H)]


def outline(g):
    h = len(g)
    w = len(g[0])
    add = []
    for y in range(h):
        for x in range(w):
            if g[y][x] != 0:
                continue
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < h and 0 <= xx < w and g[yy][xx] != 0:
                        add.append((x, y))
                        dy = 2
                        break
                else:
                    continue
                break
    for x, y in add:
        g[y][x] = CLR["outline"]


def draw_pipe():
    g = blank()
    for x in range(6, 26):
        g[4][x] = CLR["light"]
        g[5][x] = CLR["white"]
        g[6][x] = CLR["light"]
        g[7][x] = CLR["gray"]
        g[8][x] = CLR["gray"]
        g[9][x] = CLR["shadow"]
        g[10][x] = CLR["shadow"]
    for x in (10, 11, 18, 19):
        for y in range(5, 10):
            g[y][x] = CLR["rust"]
    for y in range(6, 10):
        g[y][25] = CLR["shadow"]
        g[y][26] = CLR["outline"]
        g[y][27] = CLR["shadow"]
    g[5][26] = CLR["light"]
    g[10][26] = CLR["outline"]
    outline(g)
    return g


def draw_vest():
    g = blank()
    for y in range(3, 7):
        for x in range(5, 27):
            g[y][x] = CLR["light"]
    for y in range(6, 14):
        for x in range(8, 24):
            g[y][x] = CLR["gray"]
    for y in range(7, 14):
        g[y][14] = CLR["white"]
        g[y][15] = CLR["shadow"]
        g[y][16] = CLR["shadow"]
        g[y][17] = CLR["white"]
    for y in range(3, 6):
        for x in range(13, 19):
            g[y][x] = 0
    for y in range(4, 6):
        g[y][8] = CLR["gold"]
        g[y][23] = CLR["gold"]
    outline(g)
    return g


def draw_medpack():
    g = blank()
    for y in range(3, 13):
        for x in range(8, 24):
            g[y][x] = CLR["white"]
    for y in range(5, 11):
        g[y][15] = CLR["red"]
        g[y][16] = CLR["red"]
    for x in range(12, 20):
        g[7][x] = CLR["red"]
        g[8][x] = CLR["red"]
    g[4][9] = CLR["light"]
    g[4][10] = CLR["light"]
    outline(g)
    return g


def draw_key(metal):
    g = blank()
    cx, cy = 8, 8
    for y in range(H):
        for x in range(W):
            d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy)
            if 9 <= d2 <= 25:
                g[y][x] = metal
    for x in range(13, 28):
        g[7][x] = metal
        g[8][x] = metal
    for y in range(9, 13):
        g[y][22] = metal
        g[y][23] = metal
        g[y][26] = metal
        g[y][27] = metal
    outline(g)
    return g


def scale(g, dw, dh):
    sh = len(g)
    sw = len(g[0])
    out = [[0] * dw for _ in range(dh)]
    for y in range(dh):
        for x in range(dw):
            x0 = x * sw // dw
            x1 = max(x0 + 1, (x + 1) * sw // dw)
            y0 = y * sh // dh
            y1 = max(y0 + 1, (y + 1) * sh // dh)
            counts = Counter()
            for yy in range(y0, min(sh, y1)):
                for xx in range(x0, min(sw, x1)):
                    c = g[yy][xx]
                    if c:
                        counts[c] += 1
            if counts:
                out[y][x] = counts.most_common(1)[0][0]
    return out


def be16(n):
    return bytes(((n >> 8) & 255, n & 255))


def ace_bitmap(pixels, bpp):
    h = len(pixels)
    w = len(pixels[0])
    if w % 8:
        raise SystemExit("width must be a multiple of 8")
    stride = w // 8
    out = bytearray(be16(w) + be16(h) + bytes((bpp, 0, 0, 0, 0)))
    for plane in range(bpp):
        bit = 1 << plane
        for y in range(h):
            for xb in range(stride):
                byte = 0
                for b in range(8):
                    if pixels[y][xb * 8 + b] & bit:
                        byte |= 0x80 >> b
                out.append(byte)
    return bytes(out)


def pack(pixels):
    """6-bit image: opaque pixels are 32+wallset index so the wallset bank shows."""
    h = len(pixels)
    w = len(pixels[0])
    color = [[(32 + c) if c else 0 for c in row] for row in pixels]
    mask = [[1 if c else 0 for c in row] for row in pixels]
    return ace_bitmap(color, 6), ace_bitmap(mask, 1)


def write_pair(stem, pixels):
    bm, msk = pack(pixels)
    with open(stem + ".bm", "wb") as f:
        f.write(bm)
    with open(stem + ".msk", "wb") as f:
        f.write(msk)


def main():
    os.makedirs(OUT, exist_ok=True)
    arts = [
        draw_pipe(),
        draw_vest(),
        draw_key(CLR["gold"]),
        draw_key(CLR["blue"]),
        draw_medpack(),
        draw_key(CLR["rust"]),
    ]
    for i, art in enumerate(arts):
        near = art
        far = scale(art, 16, 8)
        hud = art
        for d, img in ((0, near), (1, near), (2, far), (3, far)):
            write_pair(os.path.join(OUT, "%d_d%d" % (i, d)), img)
        write_pair(os.path.join(OUT, "%d_ui" % i), hud)
    print("wrote", len(arts), "item sprites to", OUT)


if __name__ == "__main__":
    main()
