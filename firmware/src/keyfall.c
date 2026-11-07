/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk); from Dinghy's dinghy.c by zvenson */
/* KEYFALL: a rhythm game for the M-VAVE FM-1 (README.md). This is firmware 0.1 in the making (ROADMAP.md, phase
 * 3): the synthwave engine first (kf_engine.h, measured before the game is built on it), as a SOUND CHECK:
 *
 *   keys       play the patch PRESETS picks (all 25, each on its own part's bus and sends)
 *   OCT- OCT+  the octave
 *   PRESETS    the patch; SELECT: the drum kit (MIDI channel 10)
 *   KNOB 1     GRIT (D7), KNOB 4 MIX (the player against the band), as the reference's
 *   PLAY       DEMO: the engine's worst case (kf_bench.h), every part on its gritty patch, looped
 *   MIDI in    the song channels (D3): 1 PLAY, 2 BASS, 3 PAD, 4 ARP, 5 LEAD, 6 KEYS, 10 DRUMS; notes 0-11 pick a
 *              channel's patch (D6); the clock sets the delay's tempo. USB or the TRS jack.
 *
 * Rules of the road (the chip has no FPU and no libgcc): integers only, no float, no 64-bit division.
 * tests/host.c runs this file on a computer: the screen as a PNG, the sound as a WAV.
 * What platform.c offers: docs/API.md. The chip's rules: docs/RULES.md. */
#define APP_NAME "KEYFALL"                      /* the rescue and the crash screen say this */
#define STORE_MAGIC 0x4C41464Bu                 /* "KFAL": KEYFALL's settings and blobs, never another fork's */
static void app_idle(void);
#define LCD_IDLE app_idle                       /* while the screen's DMA runs: MIDI in (platform.c) */
#include "platform.c"
#include "kf_engine.h"
#include "kf_bench.h"

static void app_audio(int32_t *out, uint32_t n) { kf_render(out, n); }

/* ---- what survives a power-off */
static struct { uint8_t oct, patch, kit, grit, mix; } set = {0, 0, 0, 70, 60};

/* ---- MIDI in (D3, D6): a channel is a part; notes 0-11 pick its patch */
static const int8_t CH_PART[16] = {0, 1, 2, 3, 4, 5, -1, -1, -1, 6, -1, -1, -1, -1, -1, -1};
static uint8_t live_patch[7];                   /* a part's patch from its keyswitch notes (an index into its own) */
static uint32_t idle_ok, midi_src, midi_seen, clk_n, clk_t0, clk_bpm_x100, clk_run;
static uint32_t ch_act[16];                     /* fm1_ms of a channel's last note */

static void midi_in(void)
{
    uint8_t st, d1, d2;
    int src;
    while ((src = midi_get(&st, &d1, &d2)) != 0) {   /* 1 USB, 2 the TRS jack */
        uint32_t ty = st & 0xF0u, ch = st & 15u;
        int32_t part;
        if (st == 0xF8u) {                      /* clock: 24 a quarter note; the tempo from a bar's worth */
            if (clk_n % 96u == 0u) {
                if (clk_n) {
                    uint32_t ms = fm1_ms - clk_t0;
                    if (ms) { clk_bpm_x100 = 24000000u / ms; kf_set_tempo(clk_bpm_x100); }
                }
                clk_t0 = fm1_ms;
            }
            clk_n++;
            continue;
        }
        if (st == 0xFAu) { clk_run = 1; clk_n = 0; continue; }
        if (st == 0xFBu) { clk_run = 1; continue; }
        if (st == 0xFCu) { kf_ev_t e = {0, KF_EV_ALLOFF, 0, 0, 0, 0, 0, 0}; clk_run = 0; kf_post(&e); continue; }
        if (ty != 0x90u && ty != 0x80u) continue;
        part = CH_PART[ch];
        if (part < 0) continue;
        midi_src = (uint32_t)src;
        midi_seen = fm1_ms | 1u;
        if (d1 < 12u) {                          /* a keyswitch: the part's patch from now on, never heard */
            if (ty == 0x90u && d2 && d1 < KF_PART_COUNT[part]) live_patch[part] = d1;
            continue;
        }
        ch_act[ch] = fm1_ms | 1u;
        if (part == (int32_t)KF_P_DRUMS) { if (ty == 0x90u && d2) kf_drum(0, live_patch[part], d1, d2); continue; }
        if (ty == 0x90u && d2) kf_note_on(0, (uint32_t)part, KF_PART_FIRST[part] + live_patch[part], d1, d2, 0, 4096);
        else kf_note_off(0, (uint32_t)part, d1);
    }
}
static void app_idle(void)
{
    if (idle_ok) midi_in();
}

/* ---- the screen */
#define C_CYAN RGB(95, 227, 255)                 /* the reference's cyan */
#define C_TEXT RGB(239, 238, 250)
#define C_MUTE RGB(143, 141, 176)
static const char *const PART_NAME[7] = {"PLAY", "BASS", "PAD", "ARP", "LEAD", "KEYS", "DRUMS"};
static uint32_t part_of(uint32_t patch)
{
    uint32_t p = 5;
    while (p && patch < KF_PART_FIRST[p]) p--;
    return p;
}
static char *put(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *num(char *p, uint32_t v)
{
    char t[10];
    int n = 0;
    do t[n++] = (char)('0' + v % 10u); while ((v /= 10u) && n < 10);
    while (n) *p++ = t[--n];
    return p;
}
static void draw_all(uint32_t demo)
{
    char s[40], *p;
    int32_t oct = (int8_t)set.oct;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 14, 240, &FONT_L, "KEYFALL", C_CYAN, 1);
    draw_text_box(0, 50, 240, &FONT_S, "SOUND CHECK  v" APP_VER, C_MUTE, 1);
    draw_text_box(0, 84, 240, &FONT_S, PART_NAME[part_of(set.patch)], C_MUTE, 1);
    draw_text_box(0, 102, 240, &FONT_L, KF_PATCH[set.patch].name, C_TEXT, 1);
    p = put(s, "KIT "); p = put(p, KF_KIT[set.kit].name); *p = 0;
    draw_text_box(0, 144, 240, &FONT_S, s, C_MUTE, 1);
    p = put(s, "GRIT "); p = num(p, set.grit); p = put(p, "  MIX "); p = num(p, set.mix);
    p = put(p, "  OCT "); if (oct < 0) *p++ = '-'; else if (oct) *p++ = '+';
    p = num(p, (uint32_t)(oct < 0 ? -oct : oct)); *p = 0;
    draw_text_box(0, 168, 240, &FONT_S, s, C_MUTE, 1);
    draw_text_box(0, 196, 240, &FONT_S, demo ? "DEMO  (PLAY stops it)" : "PLAY: the DEMO", demo ? C_CYAN : C_MUTE, 1);
}
static void draw_midi(void)
{
    char s[40], *p = s;
    uint32_t ch;
    if (midi_seen) {
        p = put(p, midi_src == 2u ? "TRS " : "USB ");
        for (ch = 0; ch < 16u; ch++)
            if (CH_PART[ch] >= 0) *p++ = ch_act[ch] && fm1_ms - ch_act[ch] < 200u ? (char)(ch == 9u ? '0' : '1' + ch) : '.';
        if (clk_bpm_x100) { p = put(p, "  "); p = num(p, (clk_bpm_x100 + 50u) / 100u); p = put(p, " BPM"); }
    }
    *p = 0;
    draw_text_box(0, 218, 240, &FONT_S, s, C_MUTE, 1);
}

static int32_t step(uint32_t enc, int32_t v, int32_t by, int32_t lo, int32_t hi, uint32_t *changed)
{
    int32_t e = fm1_enc_take(enc);
    if (!e) return v;
    *changed = 1;
    v += e * by;
    return v < lo ? lo : v > hi ? hi : v;
}

static void app_main(void)
{
    uint32_t prev = 0, k, t_change = 0, dirty = 0, demo = 0, midi_drawn = 0;
    uint8_t held[27] = {0};                     /* the note each key plays */
    if (settings_load(&set, sizeof set) || (int8_t)set.oct < -2 || (int8_t)set.oct > 2 || set.patch >= KF_NPATCH ||
        set.kit >= KF_NKIT || set.grit > 100u || set.mix > 100u) {
        set.oct = 0; set.patch = 0; set.kit = 0; set.grit = 70; set.mix = 60;
    }
    live_patch[KF_P_DRUMS] = set.kit;
    kf_init();
    kf_set_grit(set.grit);
    kf_set_mix(set.mix);
    draw_all(0);

    for (;;) {
        uint32_t keys, down, up, rel, b, part = part_of(set.patch), base, changed = 0;
        int32_t v;
        platform_poll();

        base = (uint32_t)(53 + 12 * (int8_t)set.oct);   /* key 0 = F3 */
        keys = fm1_in.notes;
        down = keys & ~prev;
        up = prev & ~keys;
        prev = keys;
        for (k = 0; k < 27u; k++) {             /* (a key stops the note it started: the octave may change meanwhile) */
            if (down >> k & 1u) { held[k] = (uint8_t)(base + k); kf_note_on(0, part, set.patch, held[k], 100, 0, 4096); }
            if (up >> k & 1u) kf_note_off(0, part, held[k]);
        }

        b = fm1_input_edges(&rel);
        if (b & (1u << BTN_OCT_DOWN) && (int8_t)set.oct > -2) { set.oct = (uint8_t)((int8_t)set.oct - 1); changed = 1; }
        if (b & (1u << BTN_OCT_UP) && (int8_t)set.oct < 2) { set.oct = (uint8_t)((int8_t)set.oct + 1); changed = 1; }
        if (b & (1u << BTN_PLAY)) {
            demo ^= 1u;
            if (demo) kf_bench_start(kf_now + 2048u);
            else { kf_ev_t ev = {0, KF_EV_ALLOFF, 0, 0, 0, 0, 0, 0}; kf_sched_clear(); kf_post(&ev); }
            draw_all(demo);
        }
        set.patch = (uint8_t)step(ENC_PRESETS, set.patch, 1, 0, (int32_t)KF_NPATCH - 1, &changed);
        set.kit = (uint8_t)step(ENC_SELECT, set.kit, 1, 0, (int32_t)KF_NKIT - 1, &changed);
        live_patch[KF_P_DRUMS] = set.kit;
        v = step(ENC_KNOB1, set.grit, 5, 0, 100, &changed);
        if (v != set.grit) { set.grit = (uint8_t)v; kf_set_grit(set.grit); }
        v = step(ENC_KNOB4, set.mix, 10, 0, 100, &changed);
        if (v != set.mix) { set.mix = (uint8_t)v; kf_set_mix(set.mix); }
        if (changed) { dirty = 1; t_change = fm1_ms; }

        midi_in();
        if (demo) kf_bench_post(kf_now + 4096u);   /* 93 ms ahead of the speaker */

        for (k = 0; k < 27u; k++) fm1_led_key(KEY_LED(k), prev >> k & 1u);   /* the keys lit while held */
        fm1_led_key(BTN_OCT_DOWN, (int8_t)set.oct < 0);
        fm1_led_key(BTN_OCT_UP, (int8_t)set.oct > 0);
        fm1_led_key(BTN_PLAY, demo != 0u);

        idle_ok = 1;                            /* the drawing below may read MIDI meanwhile (LCD_IDLE) */
        if (changed) draw_all(demo);
        if (fm1_ms - midi_drawn > 100u) { draw_midi(); midi_drawn = fm1_ms; }
        idle_ok = 0;

        if (dirty && !demo && fm1_ms - t_change > 2000u) {   /* 2 s after the change, when the demo is off */
            uint32_t q = 1;                       /* (an erase stalls the sound: save only when it is silent) */
            for (k = 0; k < KF_NV; k++) if (kf_v[k].state) q = 0;
            if (q) { settings_save(&set, sizeof set); dirty = 0; }
        }
    }
}
