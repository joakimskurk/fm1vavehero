#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Joakim (joakimskurk)
"""VaveHero's splash art and logo, as an 80s video game cover: a sunset sky with stars, a striped retro sun, mountains
with a neon rim, and the VAVEHERO logo in italic chrome with an extruded edge and a glow.

The art is the still upper part of the splash (240 x 148); the floor under the horizon (the neon grid with the notes
coming at you, the sun's reflection, PRESS PLAY) is drawn live by the firmware (kf_screens.h, ks_splash) and the
reference (drawSplash), the same way in both. The logo alone, on a clear background, is the project's mark: the README,
the reference's page, the mockup (assets/vavehero-logo.png).

Drawn at 4x and box-filtered down; the art is then rounded to RGB565, the screen's own colours, so the firmware and
the reference show the very same pixels.

    python3 tools/gen_splash.py            -> firmware/gen/kf_splash.h, assets/vavehero-logo.png, the reference's
                                              splash and logo blocks, build/splash-art.png
    python3 tools/gen_splash.py --check    exit 1 if kf_splash.h is not what this would write

The logo's letters are Inter Display Black Italic (SIL OFL 1.1, rsms.me/inter); set INTER to its .otf if it is not
in /usr/share/fonts/opentype/inter/."""
import base64
import io
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H, S = 240, 148, 4                      # the art, and the supersampling
FONT = os.environ.get("INTER", "/usr/share/fonts/opentype/inter/InterDisplay-BlackItalic.otf")


def hexc(h):
    return np.array([int(h[i:i + 2], 16) for i in (1, 3, 5)], dtype=np.float64)


def ramp(stops, t):
    """colour at t (0..1) along [(t0, '#rrggbb'), ...]"""
    t = np.clip(t, 0, 1)
    out = np.zeros(t.shape + (3,))
    for (a, ca), (b, cb) in zip(stops, stops[1:]):
        m = (t >= a) & (t <= b)
        f = ((t - a) / (b - a))[m][:, None] if b > a else 0
        out[m] = hexc(ca) * (1 - f) + hexc(cb) * f
    return out


def over(dst, col, alpha, acc=None):
    """composite a colour (H x W x 3 or 3) at alpha (H x W, 0..1) onto dst; acc: the coverage so far (the logo's own)"""
    a = np.clip(alpha, 0, 1)[..., None]
    dst[:] = dst * (1 - a) + col * a
    if acc is not None:
        acc[:] = acc + (1 - acc) * a[..., 0]


def blur(m, r):
    im = Image.fromarray(np.uint8(np.clip(m, 0, 1) * 255))
    return np.asarray(im.filter(ImageFilter.GaussianBlur(r)), dtype=np.float64) / 255


def logo(img, y, x, acc=None, top_px=16.0, width_px=206):
    """the logo onto img (supersampled), its top at top_px; acc gathers its coverage (for the clear version)"""
    hh, ww = img.shape[:2]
    font = ImageFont.truetype(FONT, 46 * S)
    text = "VAVEHERO"
    bbox = font.getbbox(text)
    scale = min(1.0, (width_px * S) / (bbox[2] - bbox[0]))   # (the italic, the extrusion and the glow need room)
    font = ImageFont.truetype(FONT, int(46 * S * scale))
    bbox = font.getbbox(text)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    ox, oy = (ww - tw) / 2 - bbox[0] - 3 * S, top_px * S - bbox[1]
    im = Image.new("L", (ww, hh), 0)
    ImageDraw.Draw(im).text((ox, oy), text, font=font, fill=255)
    face = np.asarray(im, dtype=np.float64) / 255
    top, bot = top_px, top_px + th / S
    ext = np.zeros_like(face)
    for i in range(1, 4 * S + 1):                # the extrusion: down and to the right
        ext = np.maximum(ext, np.roll(np.roll(face, i, axis=0), i // 2, axis=1))
    shape = np.maximum(face, ext)
    outline = np.clip(blur(shape, 1.3 * S) * 3.2, 0, 1)
    glow = blur(shape, 6 * S)
    over(img, hexc("#ff2fd0"), .85 * glow, acc)
    over(img, hexc("#0b0220"), outline, acc)
    tt = (y - top) / (bot - top)
    over(img, ramp([(0, "#7a2a9e"), (1, "#2a0838")], tt), ext * (1 - face), acc)
    chrome = np.where(tt[..., None] < .52,
                      ramp([(0, "#ffffff"), (.25, "#dff3ff"), (.5, "#7ec8ff")], tt / .52 * .5 * 2),
                      ramp([(0, "#20103f"), (.18, "#3a1460"), (.5, "#c2186b"), (.78, "#ff8a3d"), (1, "#ffe27a")], (tt - .52) / .48))
    over(img, chrome, face, acc)
    over(img, hexc("#ffffff"), face * .9 * np.exp(-((tt - .515) ** 2) / .0006), acc)   # the chrome's horizon line
    sx0 = (ox + bbox[0]) / S + 22                # a glint on the V
    star = np.exp(-(((x - sx0) ** 2) / .12)) * np.exp(-np.abs(y - (top + 2)) / 3.5) + \
        np.exp(-(((y - (top + 2)) ** 2) / .12)) * np.exp(-np.abs(x - sx0) / 5)
    over(img, hexc("#ffffff"), np.clip(star, 0, 1), acc)


def art():
    ys, xs = np.mgrid[0:H * S, 0:W * S].astype(np.float64)
    y, x = (ys + .5) / S, (xs + .5) / S
    img = np.zeros((H * S, W * S, 3))

    # the sky: deep night at the top, magenta, the hot horizon
    img[:] = ramp([(0, "#07021a"), (.38, "#1e0a4a"), (.66, "#5a1270"), (.84, "#c22a73"), (.95, "#ff5a6e"), (1, "#ff9a5a")], y / H)

    # stars: fixed, a few with a cross of light
    rng = np.random.default_rng(1985)
    for _ in range(95):
        sx, sy = rng.uniform(4, W - 4), rng.uniform(3, 86)
        b = rng.uniform(.35, 1) * (1 - sy / 110)
        r2 = (x - sx) ** 2 + (y - sy) ** 2
        over(img, hexc("#ffffff"), b * np.exp(-r2 / .18))
        if b > .72:
            cross = np.exp(-((x - sx) ** 2) / .03) * np.exp(-np.abs(y - sy) / 1.6) + np.exp(-((y - sy) ** 2) / .03) * np.exp(-np.abs(x - sx) / 1.6)
            over(img, hexc("#ffd6f8"), .55 * b * cross)

    # the sun: a glow, then the disc from pale yellow to hot pink, its lower half cut by bands that widen downwards
    cx, cy, R = 120.0, 116.0, 50.0
    d = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
    over(img, hexc("#ff3d9a"), .55 * np.exp(-np.maximum(d - R, 0) / 9) * (d > R - 1))
    sun = np.clip(R - d + .5, 0, 1)
    rel = (y - (cy - R)) / (2 * R)
    cut = np.zeros_like(sun)
    k, gy = 0, cy - 16
    while gy < cy + R:                           # the bands: thin high up, wider towards the horizon
        h = .8 + .5 * k
        cut = np.maximum(cut, np.clip(np.minimum(y - gy, gy + h - y) * S, 0, 1))
        gy += h + 5.2 - .45 * k
        k += 1
    over(img, ramp([(0, "#fff4a8"), (.35, "#ffd25c"), (.6, "#ff8a3d"), (.82, "#ff3d78"), (1, "#e0157a")], rel), sun * (1 - cut))

    # the mountains: a far range in violet with a magenta rim, a near one in ink with a cyan rim
    def ridge(points):
        return np.interp(x[0], [p[0] for p in points], [p[1] for p in points])
    far = ridge([(0, 126), (18, 118), (34, 124), (52, 106), (70, 122), (84, 116), (98, 128), (150, 128), (162, 112),
                 (176, 121), (192, 102), (212, 120), (226, 113), (240, 122)])
    near = ridge([(0, 138), (14, 131), (30, 140), (46, 128), (64, 143), (176, 143), (190, 130), (204, 138), (222, 125),
                  (240, 134)])
    for top, body, rim, a in ((far, "#2a0b52", "#ff4fd8", .9), (near, "#12052c", "#3fe0ff", 1)):
        t = top[None, :]
        over(img, ramp([(0, body), (1, "#07021a")], np.clip((y - t) / 24, 0, 1)), np.clip(y - t + .5, 0, 1))
        over(img, hexc(rim), a * np.exp(-((y - t) ** 2) / .35) * (y > t - 1.5))

    over(img, hexc("#ff4f9a"), .5 * np.exp(-((y - H) ** 2) / 18))   # the horizon's haze
    logo(img, y, x)
    small = img.reshape(H, S, W, S, 3).mean(axis=(1, 3))
    return np.clip(np.round(small), 0, 255).astype(np.uint8)


def logo_alone():
    """the logo on a clear background, 960 x 200 (the art's 4x): the project's mark"""
    hh, ww = 50 * S, W * S
    ys, xs = np.mgrid[0:hh, 0:ww].astype(np.float64)
    img, acc = np.zeros((hh, ww, 3)), np.zeros((hh, ww))
    logo(img, (ys + .5) / S, (xs + .5) / S, acc, top_px=8.0)
    rgb = np.where(acc[..., None] > 0, img / np.maximum(acc[..., None], 1e-6), 0)   # (un-premultiplied)
    rgba = np.dstack([np.clip(np.round(rgb), 0, 255), np.clip(np.round(acc * 255), 0, 255)]).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def to565(px):
    r, g, b = px[..., 0].astype(np.uint32), px[..., 1].astype(np.uint32), px[..., 2].astype(np.uint32)
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def from565(c):
    r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
    return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], axis=-1).astype(np.uint8)


def block(t, name, value):
    a, z = t.find(f"/*@{name}*/"), t.find(f"/*{name}@*/")
    if a < 0 or z < a:
        sys.exit(f"reference/vavehero.html has no {name} block (/*@{name}*/ ... /*{name}@*/)")
    return t[:a] + f"/*@{name}*/{value}" + t[z:]


def png64(im):
    buf = io.BytesIO()
    im.save(buf, format="PNG", optimize=True)
    return base64.b64encode(buf.getvalue()).decode(), len(buf.getvalue())


def main():
    c = to565(art())
    shown = from565(c)                           # exactly what the screen shows
    sw = ((c & 0xFF) << 8) | (c >> 8)            # the canvas keeps RGB565 byte-swapped (the panel's order)
    lines = ["/* SPDX-License-Identifier: GPL-3.0-only */",
             "/* Generated by tools/gen_splash.py: do not edit, run the tool. The splash art, 240 x 148, RGB565 as the canvas",
             " * keeps it (byte-swapped); the logo's letters are Inter Display Black Italic (SIL OFL 1.1). */",
             "#pragma once", "#include <stdint.h>", f"#define KF_SPLASH_W {W}u", f"#define KF_SPLASH_H {H}u",
             f"static const uint16_t KF_SPLASH[{W * H}] = {{"]
    flat = sw.flatten()
    for i in range(0, len(flat), 24):
        lines.append("    " + ",".join(str(int(v)) for v in flat[i:i + 24]) + ",")
    lines.append("};")
    text = "\n".join(lines) + "\n"
    dst = os.path.join(HERE, "firmware", "gen", "kf_splash.h")
    if "--check" in sys.argv:
        cur = open(dst).read() if os.path.exists(dst) else ""
        if cur != text:
            sys.exit("firmware/gen/kf_splash.h is out of date: python3 tools/gen_splash.py")
        print("kf_splash.h up to date")
        return
    open(dst, "w").write(text)
    os.makedirs(os.path.join(HERE, "build"), exist_ok=True)
    Image.fromarray(shown).save(os.path.join(HERE, "build", "splash-art.png"))
    mark = logo_alone()
    mark.save(os.path.join(HERE, "assets", "vavehero-logo.png"), optimize=True)
    art64, n_art = png64(Image.fromarray(shown))
    logo64, n_logo = png64(mark.resize((480, 100), Image.LANCZOS))
    ref = os.path.join(HERE, "reference", "vavehero.html")
    t = open(ref).read()
    t = block(t, "SPLASH", "window.VAVEHERO_SPLASH = 'data:image/png;base64," + art64 + "';")
    t = block(t, "LOGO", "window.VAVEHERO_LOGO = 'data:image/png;base64," + logo64 + "';")
    open(ref, "w").write(t)
    print(f"firmware/gen/kf_splash.h: {W} x {H}, {W * H * 2} bytes; assets/vavehero-logo.png; the reference's art "
          f"({n_art} bytes) and logo ({n_logo} bytes)")


if __name__ == "__main__":
    main()
