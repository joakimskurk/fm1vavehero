/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk) */
/* KEYFALL's screens, drawn as the reference draws them (reference/keyfall.html, "the highway", "the screens"):
 * the splash, the songs, the play (the highway with its gems, holds, chords, OCT bands, beams and bursts, the key
 * strip, the header, the OP meter, the banners) and the results. Each draws the whole screen into the canvas
 * (kf_gfx.h); keyfall.c sends it. */
#pragma once
#include "kf_gfx.h"
#include "kf_game.h"

static const char *const KS_LEVELS[4] = {"EASY", "MED", "HARD", "EXP"};
static rgb_t kjcol(uint32_t j)
{
    static const rgb_t C[7] = {K_TEXT, K_CYAN, K_BLUE, K_VIOLET, K_CORAL, K_CORAL, K_VIOLET};
    return C[j < 7u ? j : 0u];
}

/* ---- the strips at the top and the bottom */
static void ks_header(const char *num, const char *title, const char *right, int32_t prog_q12, rgb_t rcol)
{
    int32_t x = 4;
    krect(0, 0, 240, 18, K_SURF);
    if (num) { char s[8]; kput(kput(s, num), " "); x = ktext(&FONT_S, x, 1, s, K_AMBER); }
    ktext(&FONT_S, x, 1, title, K_TEXT);
    if (right) ktextR(&FONT_S, 236, 1, right, rcol);
    if (prog_q12 >= 0) { krect(0, 18, 240, 2, 0x25244au); krect(0, 18, (240 * prog_q12 + 2048) >> 12, 2, K_CYAN); }
    else krect(0, 18, 240, 1, K_LINE);
}
static void ks_knobs(const char *const *labels, uint32_t muted)   /* muted: bit i */
{
    uint32_t i;
    krect(0, 222, 240, 18, K_SURF);
    for (i = 0; i < 4u; i++) {
        int32_t cx = 30 + 60 * (int32_t)i;
        rgb_t col = muted >> i & 1u ? K_MUTE : KC[i];
        krect(cx - 22, 222, 44, 2, col);
        if (i) krect(60 * (int32_t)i, 226, 1, 11, K_LINE);
        ktextC(&FONT_S, cx, 223, labels[i], col);
    }
}
static void ks_banner(const char *big, const char *small, rgb_t col, int32_t y)
{
    krect(0, y, 240, small ? 56 : 42, kmix(K_BG, K_BLACK, Q12(.35)));
    krect(0, y, 240, 1, col);
    krect(0, y + (small ? 55 : 41), 240, 1, col);
    ktextOut(&FONT_L, KH_CX - (ktw(&FONT_L, big) >> 1), y + 4, big, col, K_BG, 1);
    if (small) ktextC(&FONT_S, KH_CX, y + 36, small, K_TEXT);
}
static void ks_pill(int32_t x, int32_t y, const char *t, int32_t on, rgb_t oncol, int32_t locked)
{
    int32_t w = ktw(&FONT_S, t) + 10;
    if (on) kpcap(x, y, w, 18, oncol);
    else { kpcap(x, y, w, 18, K_LINE); kpcap(x + 1, y + 1, w - 2, 16, K_SURF); }
    ktext(&FONT_S, x + 5, y + 1, t, on ? K_INK : locked ? K_MUTE : K_DIM);
}

/* ---- the highway */
static void ks_highway_bg(int32_t boost, int32_t spread)
{
    static const uint8_t BLACK_B[9] = {0, 1, 1, 0, 1, 1, 1, 0, 0};   /* lane lines with a black key */
    int32_t y, i;
    for (y = KH_TOPY; y < KH_HIT; y++) {
        const uint8_t *xs = kh_x[y - KH_TOPY];
        int32_t f = kh_fade[y - KH_TOPY], near = (y - KH_TOPY) * 4096 / (KH_HIT - KH_TOPY);
        for (i = 0; i < 8; i++) {
            rgb_t col = i % 2 ? 0x100f23u : 0x0c0b1bu;
            col = boost ? kmix(col, i % 2 ? 0x6a3cb0u : 0x4a2a86u, Q12(.18) + near * 901 / 4096) : kmix(col, LANE[i], Q12(.04) + near * 451 / 4096);
            krect(xs[i], y, xs[i + 1] - xs[i], 1, kmix(K_BG, col, f));
        }
        for (i = 0; i <= 8; i++) {
            int32_t edge = i == 0 || i == 8, x = xs[i] - (i == 8);
            rgb_t col = edge ? (boost ? K_GOLD : 0x4d4b96u) : spread ? 0x2a2950u : BLACK_B[i] ? 0x2b2a58u : 0x1c1b38u;
            krect(x, y, 1, 1, kmix(K_BG, col, f));
        }
    }
}
static int32_t ks_z(int32_t t96, int32_t pos) { return (t96 * 256 - pos) / 6; }   /* the depth of a time, Q12 (a beat a unit) */
static void ks_beat_lines(int32_t pos, int32_t boost)
{
    int32_t b;
    for (b = (pos + 25) / KG_BEAT + 1; ; b++) {  /* ceil(beat + .001) */
        int32_t z = ks_z(b * 96, pos), y, s, xl, xr, bar;
        if (z > KH_ZTOP_Q12) break;
        if (z < 82) continue;                    /* .02 */
        y = kh_round(kh_Yq(z));
        s = kh_s_at(2 * y + 1);
        bar = b % 4 == 0;
        xl = kh_X(KH_NL * 16, s); xr = kh_X(KH_NR * 16, s);
        krect(kh_round(xl), y, kh_round(xr - xl), 1,
              kmix(K_BG, boost ? (bar ? 0xc79a3cu : 0x6b5330u) : (bar ? 0x45438au : 0x26254du), kh_fade_at(y)));
    }
}
static void ks_tail(int32_t lane, int32_t z0, int32_t z1, rgb_t col, int32_t held)
{
    int32_t y, y1 = kh_round(kh_Yq(z1)), y0 = kh_round(kh_Yq(z0));
    rgb_t body = kmix(K_BG, col, held ? Q12(.9) : Q12(.55)), core = kmix(col, K_WHITE, Q12(.55));
    for (y = y1; y <= y0; y++) {
        int32_t s = kh_s_at(2 * y + 1), half = KH_LW * 15 * s / 100, xc = kh_X(kh_U4(lane * 256 + 128), s);   /* .15 LW s */
        if (half < 4096) half = 4096;
        krect(kh_round(xc - half), y, kh_round(xc + half) - kh_round(xc - half), 1, kmix(K_BG, body, kh_fade_at(y)));
        if (held) krect(kh_round(xc) - 1, y, 2, 1, core);
    }
}
static void ks_gem(int32_t lane, int32_t z, rgb_t col, int32_t boost, int32_t op)
{
    int32_t s = kh_sc(z) >> 4, yq = kh_Yq(z), f = kh_fade_at(kh_round(yq)), w, h, x0, y0, r;
    rgb_t halo = boost ? K_GOLD : op ? 0xfff0c0u : col;
    w = (84 * KH_LW * s / 100 + 2048) >> 12;
    h = (10 * s + 2048) >> 12;
    if (h < 3) h = 3;
    x0 = kh_round(kh_X(kh_U4(lane * 256 + 128), s) - w * 2048);
    y0 = kh_round(yq - h * 2048);
    kpcap(x0 - 1, y0 - 1, w + 2, h + 2, kmix(K_BG, kmix(K_BG, halo, op || boost ? Q12(.8) : Q12(.5)), f));
    kpcap(x0, y0, w, h, kmix(K_BG, col, f));
    r = (h + 1) / 2;
    krect(x0 + r, y0 + 1, w - 2 * r > 1 ? w - 2 * r : 1, 1, kmix(K_BG, kmix(col, K_WHITE, Q12(.6)), f));
    krect(x0 + r, y0 + h - 1, w - 2 * r > 1 ? w - 2 * r : 1, 1, kmix(K_BG, kmix(col, K_BLACK, Q12(.3)), f));
}
static void ks_chord_bar(int32_t lane_lo, int32_t lane_hi, int32_t z)
{
    int32_t s = kh_sc(z) >> 4, y = kh_round(kh_Yq(z)), xa = kh_X(kh_U4(lane_lo * 256 + 128), s), xb = kh_X(kh_U4(lane_hi * 256 + 128), s);
    krect(kh_round(xa), y, kh_round(xb - xa), 1, kmix(K_BG, K_WHITE, Q12(.4) * kh_fade_at(y) / 4096));
}
static void ks_oct_band(int32_t z, int32_t dir, int32_t auto_)
{
    static const int8_t DX[4] = {-48, -36, 36, 48};
    int32_t yc = kh_round(kh_Yq(z)), y, i, k;
    for (y = yc - 8; y < yc + 8; y++) {
        int32_t s = kh_s_at(2 * y + 1), xl = kh_round(kh_X(KH_NL * 16, s)), xr = kh_round(kh_X(KH_NR * 16, s));
        krect(xl, y, xr - xl, 1, (y == yc - 8 || y == yc + 7) ? K_VIOLET : kmix(K_BG, K_VIOLET, Q12(.26)));
    }
    ktextC(&FONT_S, KH_CX, yc - 8, auto_ ? "AUTO" : dir > 0 ? "OCT+" : "OCT-", 0xefe8ffu);
    for (k = 0; k < 4; k++)
        for (i = 0; i < 4; i++) {
            int32_t x = KH_CX + DX[k], yy = dir > 0 ? yc - 2 + i : yc + 1 - i;
            krect(x - i, yy, 1, 1, K_VIOLET);
            krect(x + i, yy, 1, 1, K_VIOLET);
        }
}
static void ks_beam(int32_t lane, rgb_t col)
{
    int32_t y;
    for (y = KH_HIT - 40; y < KH_HIT; y++) {
        int32_t s = kh_s_at(2 * y + 1), a = (y - (KH_HIT - 40)) * 4096 / 40, xc = kh_X(kh_U4(lane * 256 + 128), s);
        int32_t half = 46 * KH_LW * s / 100;     /* .46 LW s, Q12 */
        krect_a(kh_round(xc - half), y, kh_round(2 * half), 1, col, (int32_t)(((int64_t)a * a * Q12(.4)) >> 24));
    }
}
static void ks_burst(int32_t cx, int32_t cy, rgb_t col, int32_t age, int32_t big)   /* age Q12 */
{
    static const int8_t LB[4] = {5, 8, 11, 14}, LS[3] = {4, 7, 9};
    int32_t rays = big ? 9 : 5, nl = big ? 4 : 3, k, i, grow = 4096 + age * 6 / 10, al = 4096 - age * 7 / 10;
    for (k = 0; k < rays; k++) {
        uint32_t ph = (uint32_t)(2048 + (2 * k + 1) * 1024 / rays) & 4095u;   /* a = pi + (k + .5) / rays pi */
        int32_t ca = KF_SINE[(ph + 1024u) & 4095u], sa = KF_SINE[ph];
        for (i = 0; i < nl; i++) {
            int32_t t = big ? LB[i] : LS[i], px = cx + (int32_t)(((int64_t)ca * t * grow + (1 << 26)) >> 27), py = cy + (int32_t)(((int64_t)sa * t * grow + (1 << 26)) >> 27);
            krect_a(px, py, i ? 1 : 2, i ? 1 : 2, i % 2 ? col : K_WHITE, al);
        }
    }
    if (age < 2048) { krect_a(cx - 3, cy - 1, 7, 2, K_WHITE, al); krect_a(cx - 1, cy - 4, 2, 7, K_WHITE, al); }
}
/* the FM-1's keys: two rows of capsules (D1: the black row is shown and never used), or SPREAD's 15 (D2) */
static void ks_cap(int32_t cx, int32_t y, int32_t w, int32_t h, rgb_t col, int32_t state, int32_t black)   /* state: 0, 1 next, 2 lit */
{
    int32_t x = cx - w / 2;
    if (state == 2) { kpcap(x - 1, y - 1, w + 2, h + 2, kmix(K_BG, col, Q12(.55))); kpcap(x, y, w, h, col); krect(x + h / 2, y + 2, w - h, 1, kmix(col, K_WHITE, Q12(.65))); }
    else if (state == 1) { kpcap(x, y, w, h, col); kpcap(x + 1, y + 1, w - 2, h - 2, kmix(black ? 0x1a1930u : 0x2b2a46u, col, Q12(.22))); }
    else { kpcap(x, y, w, h, black ? 0x37366au : 0x3d3c64u); kpcap(x + 1, y + 1, w - 2, h - 2, black ? 0x191830u : 0x2b2a46u); }
}
static void ks_keys(const uint8_t *st, const uint8_t *led, int32_t spread)   /* st[lane]: 0 / 1 next / 2 lit; led[lane]: 0 1 2 */
{
    static const uint8_t BB[5] = {1, 2, 4, 5, 6};
    int32_t i, j;
    if (!spread) {
        for (i = 0; i < 5; i++) ks_cap(KH_NL + KH_LW * BB[i], 193, 18, 12, K_CORAL, 0, 1);
        for (i = 0; i < 8; i++) ks_cap(KH_NL + KH_LW * i + 14, 207, 25, 14, LANE[i], st[i], 0);   /* (U(i + .5), rounded as x is) */
        return;
    }
    for (j = 1; j <= 15; j++) {
        int32_t x = (29 * j - 3 + 1) / 2, y = 194, h = 26, w = 11, lv, s;   /* round(NL + j LW / 2 - 5.5) */
        rgb_t col, base;
        if (j % 2 == 0) { kpcap(x + 1, 198, w - 2, 18, 0x26254au); kpcap(x + 2, 199, w - 4, 16, 0x131226u); continue; }
        i = (j - 1) / 2; col = LANE[i]; s = st[i]; lv = led[i];
        if (s == 2) { kpcap(x - 1, y - 1, w + 2, h + 2, kmix(K_BG, col, Q12(.55))); kpcap(x, y, w, h, col); }
        else if (s == 1) { kpcap(x, y, w, h, col); kpcap(x + 1, y + 1, w - 2, h - 2, kmix(0x2b2a46u, col, Q12(.22))); }
        else { kpcap(x, y, w, h, 0x3d3c64u); kpcap(x + 1, y + 1, w - 2, h - 2, 0x2b2a46u); }
        base = s == 2 ? kmix(col, K_BLACK, Q12(.35)) : 0x1b1a30u;
        kpcap(x + 4, y + 5, 3, 12, lv == 2 ? K_WHITE : lv == 1 ? kmix(base, K_WHITE, Q12(.4)) : base);
    }
}

/* ---- the screens */
static void ks_splash(uint32_t ms, const char *version)
{
    int32_t i, x;
    krect(0, 0, 240, 240, K_BG);
    for (i = 0; i < 14; i++) {
        int32_t lane = (i * 5) % 8, sp = 30 + (i * 13) % 25, y = (int32_t)((ms * (uint32_t)sp / 1000u + (uint32_t)i * 37u) % 280u) - 30;
        kpcap(6 + lane * 29, y, 25, 9, kmix(K_BG, LANE[lane], Q12(.35)));
    }
    krect(0, 58, 240, 44, kmix(K_BG, K_BLACK, Q12(.45)));
    krect(0, 57, 240, 1, K_CYAN);
    krect(0, 102, 240, 1, K_CYAN);
    x = ktextOut(&FONT_L, 120 - (ktw(&FONT_L, "KEYFALL") >> 1), 64, "KEY", K_TEXT, K_BG, 1);
    ktextOut(&FONT_L, x, 64, "FALL", K_CYAN, K_BG, 1);
    krect(0, 108, 240, 18, kmix(K_BG, K_BLACK, Q12(.45)));
    ktextC(&FONT_S, 120, 109, "A RHYTHM GAME FOR FM-1", K_DIM);
    if ((ms / 500u) % 2u == 0u) { krect(70, 168, 100, 18, K_BG); ktextC(&FONT_S, 120, 169, "PRESS PLAY", K_AMBER); }
    krect(168, 222, 72, 18, K_BG);
    ktextR(&FONT_S, 236, 223, version, K_MUTE);
}

typedef struct { uint32_t sel, level, speed, spread, tone; int32_t best[8][2]; uint32_t mm_mode; } ks_menu_t;   /* best: acc10, -1 none */
static void ks_songs(const ks_menu_t *m, const char *tone)
{
    static const char *const KN[4] = {"LEVEL", "TONE", "SPEED", "MIX"}, *const KM[4] = {"MODE", "TONE", "BPM", "MIX"};
    uint32_t total = KF_NSONG + 1u, top, r;
    char s[40], *p;
    krect(0, 0, 240, 240, K_BG);
    p = knum(s, m->sel + 1u, 2); p = kput(p, "/"); knum(p, total, 2);
    ks_header(0, "SONGS", m->spread ? "SPREAD" : s, -1, m->spread ? K_AMBER : K_DIM);
    top = m->sel < 2u ? 0u : m->sel - 2u;
    if (total > 6u && top > total - 6u) top = total - 6u;
    if (total <= 6u) top = 0;
    for (r = 0; r < 6u && top + r < total; r++) {
        uint32_t i = top + r, sel = i == m->sel;
        int32_t y = 22 + (int32_t)r * 20, best, medals, k;
        if (sel) { krect(2, y - 1, 236, 19, K_SURF2); krect(2, y - 1, 3, 19, K_CYAN); }
        if (i == KF_NSONG) {
            ktext(&FONT_S, 9, y, "MI", K_CYAN); ktext(&FONT_S, 33, y, "MIDI MODE", sel ? K_CYAN : K_TEXT); ktextR(&FONT_S, 236, y, "DAW", K_DIM);
            continue;
        }
        knum(s, i + 1u, 2);
        ktext(&FONT_S, 9, y, s, sel ? K_CYAN : K_MUTE);
        ktext(&FONT_S, 33, y, KF_SONG[i].title, K_TEXT);
        best = i < 8u ? m->best[i][m->level ? 1 : 0] : -1;
        if (best < 0) { ktextR(&FONT_S, 236, y, "NEW", K_CYAN); continue; }
        medals = best >= 900 ? 3 : best >= 800 ? 2 : best >= 700 ? 1 : 0;
        for (k = 0; k < 3; k++) {
            int32_t mx = 181 + k * 8, my = y + 6;
            if (k < medals) { krect(mx, my, 5, 5, K_AMBER); krect(mx + 1, my + 1, 2, 1, 0xffe2b0u); }
            else { krect(mx, my, 5, 5, 0x3b3a66u); krect(mx + 1, my + 1, 3, 3, K_BG); }
        }
        p = knum(s, (uint32_t)(best + 5) / 10u, 0); kput(p, "%");
        ktextR(&FONT_S, 236, y, s, sel ? K_TEXT : K_DIM);
    }
    if (m->sel == KF_NSONG) {                    /* MIDI MODE */
        int32_t x;
        krect(2, 144, 236, 74, K_SURF);
        ktext(&FONT_S, 9, 148, "DAW > USB > FM-1", K_TEXT);
        ktext(&FONT_S, 9, 168, "CH 1 PLAY", K_CYAN); ktextR(&FONT_S, 231, 168, "2-6 10 BAND", K_VIOLET);
        x = 120 - (ktw(&FONT_S, "LIVE") + 10 + ktw(&FONT_S, "PREVIEW") + 10 + 6) / 2;
        ks_pill(x, 192, "LIVE", m->mm_mode == 0u, K_CYAN, 0);
        ks_pill(x + ktw(&FONT_S, "LIVE") + 16, 192, "PREVIEW", m->mm_mode == 1u, K_CYAN, 0);
        ks_knobs(KM, 0);
        return;
    }
    {
        const kf_song_t *so = &KF_SONG[m->sel];
        uint32_t bpm = so->bpm_x100 * m->speed / 100u, secs, i, x, tw = 0;
        int32_t lo = so->oct0, hi = so->oct0, o = so->oct0;
        secs = (uint32_t)so->end * 60u * 100u / (bpm ? bpm : 1u);
        krect(2, 144, 236, 74, K_SURF);
        p = kput(s, "BPM "); knum(p, (bpm + 50u) / 100u, 0);
        ktext(&FONT_S, 9, 148, s, K_TEXT);
        if (m->speed != 100u) { p = knum(s, m->speed, 0); kput(p, "%"); ktext(&FONT_S, 81, 148, s, K_AMBER); }
        p = knum(s, secs / 60u, 0); p = kput(p, ":"); knum(p, secs % 60u, 2);
        ktextR(&FONT_S, 231, 148, s, K_DIM);
        for (i = 0; i < so->noct; i++) { o += so->octs[i].dir; if (o < lo) lo = o; if (o > hi) hi = o; }
        p = kput(s, "C"); p = knum(p, (uint32_t)(4 + lo), 0); p = kput(p, "-C"); knum(p, (uint32_t)(5 + hi), 0);
        ktext(&FONT_S, 9, 168, "RANGE", K_DIM); ktext(&FONT_S, 57, 168, s, K_CYAN);
        p = s; *p = 0;
        if (so->noct) p = kput(p, "OCT");
        {
            uint32_t chords = 0, k2;
            for (k2 = 1; k2 < so->nchart[1]; k2++) if (so->chart[1][k2].t == so->chart[1][k2 - 1].t) chords = 1;
            if (chords) { if (p != s) p = kput(p, " "); p = kput(p, "CHORDS"); }
        }
        if (so->transpose) { if (p != s) p = kput(p, " "); p = kput(p, so->transpose > 0 ? "T+" : "T-"); knum(p, (uint32_t)(so->transpose > 0 ? so->transpose : -so->transpose), 0); }
        ktextR(&FONT_S, 231, 168, s, K_VIOLET);
        for (i = 0; i < 4u; i++) tw += (uint32_t)ktw(&FONT_S, KS_LEVELS[i]) + 10u;
        x = 120u - (tw + 18u) / 2u;
        for (i = 0; i < 4u; i++) { ks_pill((int32_t)x, 192, KS_LEVELS[i], i == m->level, K_AMBER, i > 1u); x += (uint32_t)ktw(&FONT_S, KS_LEVELS[i]) + 16u; }
    }
    (void)tone;
    ks_knobs(KN, 0);
}

static void ks_play(uint32_t ms, const uint8_t *keys_lit, const uint8_t *led, const char *num, const char *right)
{
    const kf_song_t *so = kg.song;
    int32_t pos = kg.pos, boost = kg_boosting(), i, ocn;
    uint8_t st[8] = {0};
    char s[24], *p;
    krect(0, 0, 240, 240, K_BG);
    ks_highway_bg(boost, kg.spread);
    ks_beat_lines(pos, boost);
    for (i = 0; i < 8; i++) if (keys_lit[i]) ks_beam(i, LANE[i]);
    for (i = (int32_t)kg.first; i < (int32_t)kg.n; i++) {   /* the holds' tails */
        const kf_cnote_t *n = &kg.notes[i];
        int32_t z0 = ks_z(n->t, pos), z1 = ks_z(n->t + n->len, pos), dead;
        if (z0 > KH_ZTOP_Q12) break;
        if (!(n->flags & 1u) || kg.nst[i] == KN_DONE || kg.nst[i] == KN_HIT) continue;
        if (z1 > KH_ZTOP_Q12) z1 = KH_ZTOP_Q12;
        if (z1 < 0) continue;
        dead = kg.nst[i] == KN_MISSED || kg.nst[i] == KN_BROKEN;
        ks_tail(n->lane, z0 < 0 ? 0 : z0, z1, dead ? 0x4a4866u : LANE[n->lane], kg.nst[i] == KN_HELD);
    }
    for (i = (int32_t)kg.first; i < (int32_t)kg.n; i++) {   /* chords: a bar under the notes that start together */
        const kf_cnote_t *n = &kg.notes[i];
        int32_t z = ks_z(n->t, pos), j, lo = n->lane, hi = n->lane;
        if (z > KH_ZTOP_Q12) break;
        if (z < -205 || kg.nst[i] != KN_PENDING || (i > 0 && kg.notes[i - 1].t == n->t && kg.nst[i - 1] == KN_PENDING)) continue;
        for (j = i + 1; j < (int32_t)kg.n && kg.notes[j].t == n->t; j++)
            if (kg.nst[j] == KN_PENDING) { if (kg.notes[j].lane < lo) lo = kg.notes[j].lane; if (kg.notes[j].lane > hi) hi = kg.notes[j].lane; }
        if (hi > lo) ks_chord_bar(lo, hi, z);
    }
    for (i = (int32_t)kg.first; i < (int32_t)kg.n; i++) {   /* the gems */
        const kf_cnote_t *n = &kg.notes[i];
        int32_t z = ks_z(n->t, pos), ph = kg_ph((uint32_t)i);
        if (z > KH_ZTOP_Q12) break;
        if (z < -205 || kg.nst[i] != KN_PENDING) continue;
        ks_gem(n->lane, z < 0 ? 0 : z, LANE[n->lane], boost, ph >= 0 && !kg.ph_failed[ph]);
    }
    for (i = 0; i < (int32_t)so->noct && i < (int32_t)KG_MAXO; i++) {
        int32_t z = ks_z(so->octs[i].t, pos);
        if (kg.ost[i] == KO_PENDING && z >= -410 && z <= KH_ZTOP_Q12) ks_oct_band(z < 0 ? 0 : z, so->octs[i].dir, kg.level == 0 || kg.midi_used);
    }
    krect(KH_NL, KH_HIT - 2, KH_NR - KH_NL, 1, kmix(K_BG, boost ? K_GOLD : K_CYAN, Q12(.4)));
    krect(KH_NL, KH_HIT - 1, KH_NR - KH_NL, 2, boost ? K_GOLD : K_CYAN);
    for (i = 0; i < 8 && i < (int32_t)kg.nfx; i++) {
        uint32_t k = (kg.nfx - 1u - (uint32_t)i) % 8u, age = ms - kg.fx[k].ms;
        if (age < 260u) ks_burst(KH_NL + KH_LW * kg.fx[k].lane + 15, KH_HIT - 1, kg.fx[k].big ? LANE[kg.fx[k].lane] : K_GREEN, (int32_t)(age * 4096u / 260u), kg.fx[k].big);
    }
    for (i = 0; i < 8; i++) st[i] = keys_lit[i] ? 2 : 0;
    for (i = (int32_t)kg.first; i < (int32_t)kg.n; i++) {   /* the next notes light their keys */
        const kf_cnote_t *n = &kg.notes[i];
        if ((int32_t)n->t * 256 > pos + KG_BEAT) break;
        if (kg.nst[i] == KN_PENDING && (int32_t)n->t * 256 >= pos - KG_BEAT / 10 && !st[n->lane]) st[n->lane] = 1;
    }
    ks_keys(st, led, kg.spread);
    if (kg.judge && ms - kg.judge_ms < 450u) {
        const char *jt = KJ_TEXT[kg.judge];
        int32_t y = 150 - (int32_t)((ms - kg.judge_ms) * 4u / 450u);
        ktextOut(&FONT_S, KH_CX - (ktw(&FONT_S, jt) >> 1), y, jt, kjcol(kg.judge), K_BG, 1);
    }
    {
        int32_t prog = pos <= 0 ? 0 : pos / (((int32_t)so->end * KG_BEAT) >> 12);   /* Q12 (32-bit) */
        ks_header(num, so->title, right ? right : KS_LEVELS[kg.level], prog > 4096 ? 4096 : prog, right ? K_MUTE : K_AMBER);
    }
    knum(s, (uint32_t)kg.score, 6);
    ktext(&FONT_S, 4, 21, s, K_TEXT);
    {                                            /* the OP meter */
        int32_t n = kg_ops_shown(), x0 = 120 - (6 * 12 + 5 * 2) / 2;
        rgb_t c = boost ? K_GOLD : K_AMBER;
        for (i = 0; i < 6; i++) {
            int32_t x = x0 + i * 14, y = 22;
            char d[2] = {(char)('1' + i), 0};
            if (i < n) { krect(x, y, 12, 14, c); krect(x + 1, y + 1, 10, 1, kmix(c, K_WHITE, Q12(.6))); }
            else { krect(x, y, 12, 14, 0x3b3a66u); krect(x + 1, y + 1, 10, 12, 0x14132au); }
            ktext(&FONT_S, x + 2, y - 1, d, i < n ? K_INK : K_MUTE);
        }
    }
    p = kput(s, "X"); knum(p, (uint32_t)(kg_mult() * (boost ? 2 : 1)), 0);
    kpcap(204, 21, 32, 16, boost ? K_GOLD : K_AMBER);
    ktextC(&FONT_S, 220, 21, s, K_INK);
    ktext(&FONT_S, 4, 40, "COMBO", K_DIM);
    knum(s, (uint32_t)kg.combo, 0);
    ktext(&FONT_L, 4, 54, s, boost ? K_GOLD : K_TEXT);
    ocn = kg_oct_at(pos);
    if (ocn != 0) ktext(&FONT_S, 4, 92, ocn > 0 ? "OCT+" : "OCT-", (kg.oct == ocn || kg.level == 0 || kg.midi_used) ? K_VIOLET : K_CORAL);
    {
        int32_t banner_up = boost && pos - kg.boost_from < 3 * KG_BEAT;
        if (kg.plus_ms && ms - kg.plus_ms < 700u && !banner_up) ktextOut(&FONT_S, 108, 38, "+OP", K_AMBER, K_BG, 1);
        if (banner_up) {
            krect(56, 44, 128, 30, kmix(K_BG, K_BLACK, Q12(.3)));
            krect(56, 43, 128, 1, K_GOLD); krect(56, 74, 128, 1, K_GOLD);
            ktextOut(&FONT_L, KH_CX - (ktw(&FONT_L, "OP BOOST") >> 1), 42, "OP BOOST", K_GOLD, K_BG, 1);
        }
    }
    {
        static const char *const KN[4] = {"SWEEP", "TONE", "SPEED", "MIX"};
        ks_knobs(KN, 1u);
    }
    if (pos < 4 * KG_BEAT) {
        p = kput(s, KS_LEVELS[kg.level]);
        if (kg.speed != 100u) { p = kput(p, " "); p = knum(p, kg.speed, 0); kput(p, "%"); }
        { uint32_t n = 0; while (so->title[n]) n++; ks_banner(n > 14u ? num : so->title, s, K_CYAN, 92); }
    } else if (pos < 8 * KG_BEAT) {
        knum(s, (uint32_t)(8 - pos / KG_BEAT), 0);
        ks_banner(s, 0, K_AMBER, 99);
    }
    if (kg.done) ks_banner(kg.full ? "FULL COMBO" : "CLEAR", 0, kg.full ? K_GOLD : K_GREEN, 99);
}

static void ks_results(const char *num)
{
    const kf_song_t *so = kg.song;
    char s[24], *p;
    const char *const NAME[4] = {"PERFECT", "GREAT", "GOOD", "MISS"};
    const int32_t V[4] = {kg.cnt[0] + kg.cnt[4], kg.cnt[1], kg.cnt[2], kg.cnt[3]};
    const rgb_t COL[4] = {K_CYAN, K_BLUE, K_VIOLET, K_CORAL};
    int32_t i, x, m;
    char g[2] = {(char)kg.grade, 0};
    krect(0, 0, 240, 240, K_BG);
    ks_header(num, so->title, KS_LEVELS[kg.level], -1, K_AMBER);
    ktextOut(&FONT_L, 10, 14, g, kg.grade == 'S' ? K_GOLD : K_AMBER, 0x3a2606u, 2);
    ktext(&FONT_S, 62, 24, "ACCURACY", K_DIM);
    p = knum(s, (uint32_t)kg.acc10 / 10u, 0); p = kput(p, "."); p = knum(p, (uint32_t)kg.acc10 % 10u, 0); kput(p, "%");
    ktext(&FONT_L, 62, 38, s, K_TEXT);
    knum(s, (uint32_t)kg.score, 6);
    ktext(&FONT_S, 62, 72, s, K_DIM);
    if (kg.new_best) { int32_t nb = ktw(&FONT_S, "NEW BEST") + 10; kpcap(236 - nb, 72, nb, 17, K_CYAN); ktext(&FONT_S, 241 - nb, 72, "NEW BEST", K_INK); }
    for (i = 0; i < 4; i++) {
        int32_t y = 96 + i * 18;
        ktext(&FONT_S, 4, y, NAME[i], COL[i]);
        krect(70, y + 7, 128, 4, 0x1d1c38u);
        if (V[i] && kg.total) { int32_t w = (128 * V[i] + kg.total / 2) / kg.total; krect(70, y + 7, w < 3 ? 3 : w, 4, COL[i]); }
        knum(s, (uint32_t)V[i], 0);
        ktextR(&FONT_S, 236, y, s, K_TEXT);
    }
    krect(2, 172, 236, 46, K_SURF);
    ktext(&FONT_S, 9, 176, "MAX COMBO", K_DIM);
    knum(s, (uint32_t)kg.max_combo, 0);
    ktextR(&FONT_S, 231, 176, s, K_TEXT);
    m = kg.dt_n ? (kg.dt_sum >= 0 ? (kg.dt_sum + kg.dt_n / 2) / kg.dt_n : -((-kg.dt_sum + kg.dt_n / 2) / kg.dt_n)) : 0;
    ktext(&FONT_S, 9, 196, "TIMING", K_DIM);
    if (m == 0) kput(s, "ON TIME");
    else { p = kput(s, m > 0 ? "+" : "-"); p = knum(p, (uint32_t)(m > 0 ? m : -m), 0); kput(p, m < 0 ? " MS EARLY" : " MS LATE"); }
    ktextR(&FONT_S, 231, 196, s, K_TEXT);
    krect(0, 222, 240, 18, K_SURF);
    x = ktext(&FONT_S, 6, 223, "PLAY", 0xffa52au);
    ktext(&FONT_S, x, 223, " RETRY", K_TEXT);
    x = 234 - ktw(&FONT_S, "HOME SONGS");
    x = ktext(&FONT_S, x, 223, "HOME", K_DIM);
    ktext(&FONT_S, x, 223, " SONGS", K_TEXT);
}
static void ks_toast(const char *t)
{
    int32_t w = ktw(&FONT_S, t) + 20, x = 120 - w / 2;
    krect(x, 96, w, 28, K_LINE);
    krect(x + 1, 97, w - 2, 26, K_SURF2);
    ktextC(&FONT_S, 120, 102, t, K_TEXT);
}
