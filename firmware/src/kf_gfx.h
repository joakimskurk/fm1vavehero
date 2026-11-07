/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk) */
/* VaveHero's drawing on Dinghy's canvas (gfx.c), the whole screen at once (CV_MAX 240 x 240): the reference's
 * pixel shapes (reference/vavehero.html, "pixel shapes", "the highway", "Terminus"), in integers.
 *
 *   colours   0xRRGGBB as the reference writes them; kmix() is its mix(), c565() the screen's
 *   text      Terminus, alpha-blended onto what is there (the reference's text, textOut, textC, textR), scale 1 or 2
 *   pcap      the capsule (keys, gems, pills), rounded as the reference rounds it
 *   highway   the perspective: Y(z) for a depth, the lanes' x at a row, the fade at the top */
#pragma once
#include <stdint.h>

typedef uint32_t rgb_t;
/* the reference's palette (P), the knobs' colours (KC), the lanes (LANE) */
#define K_BG 0x0a0a14u
#define K_SURF 0x17162bu
#define K_SURF2 0x24233fu
#define K_LINE 0x2c2b52u
#define K_TEXT 0xefeefau
#define K_DIM 0x8f8db0u
#define K_MUTE 0x5d5b8au
#define K_CYAN 0x5fe3ffu
#define K_AMBER 0xffb347u
#define K_VIOLET 0xb388ffu
#define K_CORAL 0xff6b5au
#define K_BLUE 0x5b9bffu
#define K_GOLD 0xffd36bu
#define K_INK 0x1a1206u
#define K_GREEN 0x6ee7a0u
#define K_WHITE 0xffffffu
#define K_BLACK 0x000000u
static const rgb_t KC[4] = {0x5b9be0u, 0xf59a3au, 0xf2e4c4u, 0xee6048u};
static const rgb_t LANE[8] = {0x2ee6c8u, 0x3fc1ffu, 0x5b9bffu, 0x8b7dffu, 0xb86cffu, 0xff5fc8u, 0xff7a59u, 0xffb43du};

static inline uint16_t c565(rgb_t c) { return (uint16_t)(((c >> 19) & 31u) << 11 | ((c >> 10) & 63u) << 5 | ((c >> 3) & 31u)); }
static rgb_t kmix(rgb_t a, rgb_t b, int32_t t)   /* a + (b - a) t, t Q12, each channel rounded (the reference's mix) */
{
    rgb_t o = 0;
    int32_t sh;
    for (sh = 16; sh >= 0; sh -= 8) {
        int32_t va = (int32_t)(a >> sh) & 255, vb = (int32_t)(b >> sh) & 255, v = va + (((vb - va) * t + 2048) >> 12);
        o |= (rgb_t)(v < 0 ? 0 : v > 255 ? 255 : v) << sh;
    }
    return o;
}
#define Q12(x) ((int32_t)((x) * 4096 + .5))     /* a constant fraction, Q12 (folded by the compiler: no float at run time) */

/* ---- pixels (the canvas keeps RGB565, byte-swapped) */
static inline rgb_t kpx_get(int32_t x, int32_t y)
{
    uint32_t c = swap16(cv_px[(uint32_t)y * cv_w + (uint32_t)x]);
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u;
    return (r << 3 | r >> 2) << 16 | (g << 2 | g >> 4) << 8 | (b << 3 | b >> 2);
}
/* a pixel blended in RGB565 itself (the canvas's own form: a little coarser than the reference's 8-bit mix, and
 * much cheaper), a in 1/64 */
static inline uint16_t kblend565(uint16_t under_sw, uint32_t c, uint32_t a)
{
    uint32_t u = swap16(under_sw);
    int32_t ur = (int32_t)(u >> 11), ug = (int32_t)((u >> 5) & 63u), ub = (int32_t)(u & 31u);
    int32_t cr = (int32_t)(c >> 11), cg = (int32_t)((c >> 5) & 63u), cb = (int32_t)(c & 31u), aa = (int32_t)a;
    ur += ((cr - ur) * aa + 32) >> 6; ug += ((cg - ug) * aa + 32) >> 6; ub += ((cb - ub) * aa + 32) >> 6;
    return swap16((uint32_t)ur << 11 | (uint32_t)ug << 5 | (uint32_t)ub);
}
static inline void kpx_text(int32_t x, int32_t y, uint32_t c565v, uint32_t v)   /* a glyph's pixel, v 1..15 */
{
    uint16_t *px;
    if ((uint32_t)x >= cv_w || (uint32_t)y >= cv_h) return;
    px = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    *px = v == 15u ? swap16(c565v) : kblend565(*px, c565v, v * 64u / 15u);
}
static inline void kpx_blend(int32_t x, int32_t y, rgb_t c, int32_t a)   /* a Q12 */
{
    uint16_t *px;
    if ((uint32_t)x >= cv_w || (uint32_t)y >= cv_h || a <= 0) return;
    px = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    *px = a >= 4096 ? swap16(c565(c)) : kblend565(*px, c565(c), (uint32_t)a >> 6);
}
static void krect(int32_t x, int32_t y, int32_t w, int32_t h, rgb_t c) { if (w > 0 && h > 0) cv_rect(x, y, w, h, c565(c)); }
static void krect_a(int32_t x, int32_t y, int32_t w, int32_t h, rgb_t c, int32_t a)   /* blended, a Q12 */
{
    int32_t i, j;
    uint32_t cc = c565(c), aa = (uint32_t)a >> 6;
    if (a <= 0) return;
    for (j = y < 0 ? 0 : y; j < y + h && j < (int32_t)cv_h; j++)
        for (i = x < 0 ? 0 : x; i < x + w && i < (int32_t)cv_w; i++) {
            uint16_t *px = &cv_px[(uint32_t)j * cv_w + (uint32_t)i];
            *px = a >= 4096 ? swap16(cc) : kblend565(*px, cc, aa);
        }
}

/* ---- the capsule: each row inset as a circle of radius min(w, h) / 2 would (x 0.92), standing ones round at the
 * top and the bottom; worked in half pixels with an integer square root */
static uint32_t kisqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1;
        b >>= 2;
    }
    return r;
}
static void kpcap(int32_t x, int32_t y, int32_t w, int32_t h, rgb_t c)
{
    int32_t j, d = w < h ? w : h;                /* d = 2r */
    uint16_t sc;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    d = w < h ? w : h;
    sc = c565(c);
    for (j = 0; j < h; j++) {
        int32_t yc2 = 2 * j + 1, dy2, ins = 0;  /* half pixels: yc = j + .5, r = d / 2 */
        if (h <= w) dy2 = yc2 > h ? yc2 - h : h - yc2;
        else dy2 = yc2 < d ? d - yc2 : yc2 > 2 * h - d ? yc2 - (2 * h - d) : 0;
        if (d >= 4) {                            /* r >= 2 */
            int32_t s16 = dy2 >= d ? 0 : (int32_t)kisqrt((uint32_t)(d * d - dy2 * dy2) * 256u);
            ins = (23 * (16 * d - s16) + 400) / 800;   /* round(.92 (r - sqrt(r^2 - dy^2))) */
        }
        cv_rect(x + ins, y + j, w - 2 * ins < 1 ? 1 : w - 2 * ins, 1, sc);
    }
}

/* ---- text: Terminus as cv_text has it, but blended onto what is under it, and at scale 1 or 2 */
static int32_t ktext_n(const felucca_font_t *f, int32_t x, int32_t y, const char *s, uint32_t n, rgb_t col, int32_t sc)
{
    uint32_t k, cc = c565(col);
    for (k = 0; k < n && s[k]; k++) {
        uint32_t gi = glyph(f, (uint8_t)s[k]), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
        if (s[k] != ' ')
            for (gy = 0; gy < f->h; gy++)
                for (gx = 0; gx < w; gx++) {
                    uint32_t v = gd[gy * bpr + gx / 2u];
                    v = (gx & 1u) ? (v & 15u) : (v >> 4);
                    if (v) {
                        int32_t px = x - f->pad * sc + (int32_t)gx * sc, py = y + (int32_t)gy * sc, ii, jj;
                        for (jj = 0; jj < sc; jj++) for (ii = 0; ii < sc; ii++) kpx_text(px + ii, py + jj, cc, v);
                    }
                }
        x += f->adv[gi] * sc;
    }
    return x;
}
static int32_t ktext(const felucca_font_t *f, int32_t x, int32_t y, const char *s, rgb_t col) { return ktext_n(f, x, y, s, 999u, col, 1); }
static int32_t ktw(const felucca_font_t *f, const char *s) { return text_w(f, s); }
static void ktextC(const felucca_font_t *f, int32_t cx, int32_t y, const char *s, rgb_t col)
{
    ktext(f, cx - (ktw(f, s) >> 1), y, s, col);  /* round(cx - w / 2) */
}
static void ktextR(const felucca_font_t *f, int32_t rx, int32_t y, const char *s, rgb_t col) { ktext(f, rx - ktw(f, s), y, s, col); }
static int32_t ktextOut(const felucca_font_t *f, int32_t x, int32_t y, const char *s, rgb_t col, rgb_t out, int32_t sc)
{
    static const int8_t D8[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, 1}, {-1, 1}, {1, -1}};
    uint32_t k;
    for (k = 0; k < 8u; k++) ktext_n(f, x + D8[k][0], y + D8[k][1], s, 999u, out, sc);
    return ktext_n(f, x, y, s, 999u, col, sc);
}

/* ---- numbers and strings, small */
static char *kput(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char *knum(char *p, uint32_t v, int32_t pad)   /* decimal, zero-padded to pad digits */
{
    char t[12];
    int32_t n = 0;
    do t[n++] = (char)('0' + v % 10u); while ((v /= 10u) && n < 11);
    while (n < pad) t[n++] = '0';
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}

/* ---- the highway (reference/vavehero.html, "the highway"): HIT 190, TOPY 40, lanes from 4 to 236 (29 px), the eye 3
 * deep; a row's scale s = (y - HY) / (HIT - HY), the depth's row Y(z) = HY + (HIT - HY) 3 / (3 + z) */
#define KH_HIT 190
#define KH_TOPY 40
#define KH_NL 4
#define KH_NR 236
#define KH_CX 120
#define KH_LW 29
#define KH_HYQ (-4317647)                        /* HY = 190 - 150 / (1 - 96 / 232) = -65.882, Q16 */
#define KH_DENQ 16769647                         /* HIT - HY = 255.882, Q16 */
#define KH_ZTOP_Q12 17408                        /* the far end: 3 / (96 / 232) - 3 = 4.25, Q12 */
#define KH_ROWS (KH_HIT - KH_TOPY)
static int16_t kh_s[KH_ROWS];                    /* a row's scale at its middle (y + .5), Q12 */
static uint8_t kh_x[KH_ROWS][9];                 /* a row's lane edges, round(X(U(b), s)) */
static int16_t kh_fade[KH_ROWS];                 /* clamp((y - TOPY) / 26, 0, 1), Q12 */
static uint16_t kh_lane[2][KH_ROWS][8];          /* the lanes' colours a row (normal, boost), swapped RGB565 */
static uint16_t kh_line[3][KH_ROWS][9];          /* the lane lines' (normal, boost, SPREAD) */
static int32_t kh_s_at(int32_t y2)               /* the scale at y2 / 2 (half pixels), Q12 */
{
    return (y2 * 32768 - KH_HYQ) / (KH_DENQ >> 12);   /* (32-bit: y2 < 480) */
}
static int32_t kh_X(int32_t u_q4, int32_t s_q12) /* X(u, s) = CX + (u - CX) s, u in 1/16 px: x Q12 */
{
    return KH_CX * 4096 + (((u_q4 - KH_CX * 16) * s_q12) >> 4);
}
static int32_t kh_round(int32_t v_q12) { return v_q12 >= 0 ? (v_q12 + 2048) >> 12 : -((-v_q12 + 2047) >> 12); }
static int32_t kh_U4(int32_t p_q8) { return KH_NL * 16 + ((KH_LW * 16 * p_q8) >> 8); }   /* U(p) = NL + p LW, p Q8, in 1/16 px */
static int32_t kh_sc(int32_t z_q12)              /* sc(z) = 3 / (3 + z), Q16 */
{
    return (int32_t)(805306368u / (uint32_t)(12288 + (z_q12 < -6000 ? -6000 : z_q12)));
}
static int32_t kh_Yq(int32_t z_q12)              /* Y(z), Q12 */
{
    return (int32_t)(((int64_t)KH_HYQ + (((int64_t)(KH_DENQ >> 4) * kh_sc(z_q12)) >> 12)) >> 4);
}
static int32_t kh_fade_at(int32_t y)             /* the fade at a row, Q12 */
{
    int32_t f = (y - KH_TOPY) * 4096 / 26;
    return f < 0 ? 0 : f > 4096 ? 4096 : f;
}
static void kh_init(void)
{
    static const uint8_t BLACK_B[9] = {0, 1, 1, 0, 1, 1, 1, 0, 0};   /* lane lines with a black key */
    int32_t y, b, v;
    for (y = KH_TOPY; y < KH_HIT; y++) {
        int32_t s = kh_s_at(2 * y + 1), r = y - KH_TOPY, f = kh_fade_at(y), near = r * 4096 / KH_ROWS;
        kh_s[r] = (int16_t)s;
        kh_fade[r] = (int16_t)f;
        for (b = 0; b <= 8; b++) kh_x[r][b] = (uint8_t)kh_round(kh_X(kh_U4(b * 256), s));
        for (b = 0; b < 8; b++)                  /* (the reference's highwayBg) */
            for (v = 0; v < 2; v++) {
                rgb_t col = b % 2 ? 0x100f23u : 0x0c0b1bu;
                col = v ? kmix(col, b % 2 ? 0x6a3cb0u : 0x4a2a86u, Q12(.18) + near * 901 / 4096) : kmix(col, LANE[b], Q12(.04) + near * 451 / 4096);
                kh_lane[v][r][b] = swap16(c565(kmix(K_BG, col, f)));
            }
        for (b = 0; b <= 8; b++)
            for (v = 0; v < 3; v++) {
                int32_t edge = b == 0 || b == 8;
                rgb_t col = edge ? (v == 1 ? K_GOLD : 0x4d4b96u) : v == 2 ? 0x2a2950u : BLACK_B[b] ? 0x2b2a58u : 0x1c1b38u;
                kh_line[v][r][b] = swap16(c565(kmix(K_BG, col, f)));
            }
    }
}
