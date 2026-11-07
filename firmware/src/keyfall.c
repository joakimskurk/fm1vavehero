/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk); from Dinghy's dinghy.c by zvenson */
/* KEYFALL: a rhythm game for the M-VAVE FM-1 (README.md, docs/konsept.md). The notes fall down a highway, a lane
 * a white key; you play them on the keys, the key LEDs show what comes. The browser reference
 * (reference/keyfall.html) is the fasit for how it plays, looks and sounds; this is it on the FM-1.
 *
 *   splash     PLAY: the songs. FX: the DEMO (ENGINE TOUR from bar 41 on autoplay: the engine's worst case,
 *              which fm1-emu times in CI); HOME or PLAY ends it
 *   songs      SELECT the song (the last row: MIDI MODE), KNOB 1 the level (EASY, MED), KNOB 2 the voice (TONE:
 *              SONG or one of five), KNOB 3 the speed (50-100 %), KNOB 4 MIX, ALGORITHM GRIT (D7); PLAY starts, GLO
 *              turns SPREAD on and off (D2), SEQ AUTO (the song plays itself, its result is not kept), HOME back
 *   play       the eight keys of the layout play the lanes (C4..C5, or SPREAD's every other white key); OCT- OCT+
 *              follow the octave shifts; FX: OP BOOST when the six OPs are lit; PLAY pauses, HOME quits
 *   results    PLAY again, HOME the songs; the best result of each song and level is kept in flash
 *   MIDI MODE  LIVE (D5): the FM-1 is the song's sound module, a part a channel (D3), keyswitches on notes 0-11
 *              (D6); the keys play channel 1's voice and go out on channel 1 for the DAW to record; HOME leaves
 *
 * The time: the song runs on the audio clock (kf_now, frames), and a key is judged by the frame that was being heard
 * when it went down: its edge is read in LCD_IDLE too (while the screen's DMA runs), so a long frame does not make
 * it late.
 *
 * Rules of the road (the chip has no FPU and no libgcc): integers only, no float, no 64-bit division.
 * tests/host.c runs this file on a computer: the screen as a PNG, the sound as a WAV.
 * What platform.c offers: docs/API.md. The chip's rules: docs/RULES.md. */
#define APP_NAME "KEYFALL"                      /* the rescue and the crash screen say this */
#define STORE_MAGIC 0x4C41464Bu                 /* "KFAL": KEYFALL's settings and blobs, never another fork's */
#define CV_MAX (240u * 240u)                    /* the whole screen in one canvas (113 KB of the pool) */
static void app_idle(void);
#define LCD_IDLE app_idle                       /* while the screen's DMA runs: keys and MIDI in (platform.c) */
#include "platform.c"
#include "kf_engine.h"
#include "kf_song.h"
#include "kf_bench.h"
#include "kf_screens.h"

/* ---- the sound, and the audio clock as heard */
static volatile uint32_t au_frames, au_ms;     /* kf_now and fm1_ms when the last block was rendered */
static void app_audio(int32_t *out, uint32_t n)
{
    kf_render(out, n);
    au_frames = kf_now;
    au_ms = fm1_ms;
}
#define KF_LATENCY 512u                          /* frames between a rendered block and the speaker (two halves) */
static int32_t offset_ms;                        /* the player's own latency (the calibration) */
static uint32_t heard_now(void)                  /* the frame being heard now */
{
    uint32_t f, m, f2;
    do { f = au_frames; m = au_ms; f2 = au_frames; } while (f != f2);
    return f - KF_LATENCY + (fm1_ms - m) * 441u / 10u - (uint32_t)(offset_ms * 441 / 10);
}

/* ---- what survives a power-off: the settings and the best results */
#define NBEST 16u
static struct {
    uint8_t level, speed, tone, mix, grit, spread, sel, pad;
    int16_t offset, pad2;
    uint16_t best_hash[NBEST];                   /* the song's hash | level << 14 */
    int16_t best_acc[NBEST];                     /* accuracy x 10 */
} set;
static void set_defaults(void)
{
    uint32_t i;
    set.level = 1; set.speed = 100; set.tone = 0; set.mix = 60; set.grit = 70; set.spread = 0; set.sel = 0; set.offset = 0;
    for (i = 0; i < NBEST; i++) { set.best_hash[i] = 0xFFFFu; set.best_acc[i] = -1; }
}
static int32_t best_of(uint32_t song, uint32_t level)
{
    uint32_t i, key = KF_SONG[song].hash | level << 14;
    for (i = 0; i < NBEST; i++) if (set.best_hash[i] == key) return set.best_acc[i];
    return -1;
}
static int32_t best_put(uint32_t song, uint32_t level, int32_t acc)   /* 1: a new best */
{
    uint32_t i, key = KF_SONG[song].hash | level << 14, freei = NBEST;
    for (i = 0; i < NBEST; i++) {
        if (set.best_hash[i] == key) { if (acc > set.best_acc[i]) { set.best_acc[i] = (int16_t)acc; return 1; } return 0; }
        if (set.best_hash[i] == 0xFFFFu && freei == NBEST) freei = i;
    }
    if (freei == NBEST) freei = (song * 2u + level) % NBEST;   /* full (songs that are gone): take a place */
    set.best_hash[freei] = (uint16_t)key;
    set.best_acc[freei] = (int16_t)acc;
    return 1;
}

/* ---- the player's voice: KNOB 2's TONE, or what channel 1 asks for (SONG) */
static uint32_t tone_patch(void) { return set.tone ? KF_PART_FIRST[KF_P_PLAY] + set.tone - 1u : kf_seq_play_patch(); }
static const char *tone_name(void) { return set.tone ? KF_PATCH[KF_PART_FIRST[KF_P_PLAY] + set.tone - 1u].name : "SONG"; }
static uint8_t key_note[27];                     /* the pitch each key sounds (for its release) */
static void kg_sound_on(uint32_t key, uint32_t pitch, int32_t detune, uint32_t bright)
{
    if (key >= 27u) return;
    if (key_note[key]) kf_note_off(0, KF_P_PLAY, key_note[key]);
    key_note[key] = (uint8_t)pitch;
    kf_note_on(0, KF_P_PLAY, tone_patch(), pitch, 104, detune, bright);
}
static void kg_sound_off(uint32_t key, uint32_t pitch)
{
    (void)pitch;
    if (key < 27u && key_note[key]) { kf_note_off(0, KF_P_PLAY, key_note[key]); key_note[key] = 0; }
}

/* ---- the screens and their state */
enum { SCR_SPLASH, SCR_SONGS, SCR_PLAY, SCR_RESULTS, SCR_MIDI };
static uint32_t scr, paused, paused_at, dirty_set, t_set, redraw = 1, demo, autoplay;
static char toast_s[24];
static uint32_t toast_ms;
static void toast(const char *s) { kput(toast_s, s); toast_ms = fm1_ms | 1u; redraw = 1; }
static int32_t mm_oct;                           /* MIDI MODE: the keys' octave */
static const char *const PART_NAME7[7] = {"PLAY", "BASS", "PAD", "ARP", "LEAD", "KEYS", "DRUMS"};

/* MIDI in (D3, D6): in MIDI MODE a channel is a part; in a song channel 1's notes are FM-1 keys */
static const int8_t CH_PART[16] = {0, 1, 2, 3, 4, 5, -1, -1, -1, 6, -1, -1, -1, -1, -1, -1};
static uint8_t live_patch[7];
static uint32_t idle_ok, midi_seen, clk_n, clk_t0, clk_bpm_x100, ch_act[16];
static void kg_press(uint32_t key, uint32_t frame);
static void midi_in(void)
{
    uint8_t st, d1, d2;
    while (midi_get(&st, &d1, &d2)) {
        uint32_t ty = st & 0xF0u, ch = st & 15u, on = ty == 0x90u && d2, off = ty == 0x80u || (ty == 0x90u && !d2);
        int32_t part;
        if (st == 0xF8u) {                      /* clock: the delay's tempo in MIDI MODE, from a bar's worth */
            if (clk_n % 96u == 0u) {
                if (clk_n && fm1_ms != clk_t0) { clk_bpm_x100 = 24000000u / (fm1_ms - clk_t0); if (scr == SCR_MIDI) kf_set_tempo(clk_bpm_x100); }
                clk_t0 = fm1_ms;
            }
            clk_n++;
            continue;
        }
        if (st == 0xFAu) { clk_n = 0; continue; }
        if (st == 0xFCu) { if (scr == SCR_MIDI) { kf_ev_t e = {0, KF_EV_ALLOFF, 0, 0, 0, 0, 0, 0}; kf_post(&e); } continue; }
        if (!on && !off) continue;
        if (scr == SCR_PLAY && ch == 0u && !paused && !demo) {   /* a MIDI keyboard plays the game: notes as FM-1 keys */
            int32_t key = (int32_t)d1 - 53 - 12 * kg_oct_at(kg.pos);
            if (d1 < 12u) continue;
            kg.midi_used = 1;
            if (key >= 0 && key < 27) { if (on) kg_press((uint32_t)key, heard_now()); else kg_release((uint32_t)key, heard_now()); }
            continue;
        }
        if (scr != SCR_MIDI) continue;
        part = CH_PART[ch];
        if (part < 0) continue;
        midi_seen = fm1_ms | 1u;
        if (d1 < 12u) {                          /* a keyswitch (D6); on channel 1, note 11 is the OP note */
            if (on && d1 < KF_PART_COUNT[part] && !(ch == 0u && d1 == 11u)) live_patch[part] = d1;
            continue;
        }
        ch_act[ch] = fm1_ms | 1u;
        if (part == (int32_t)KF_P_DRUMS) { if (on) kf_drum(0, live_patch[part] < KF_NKIT ? live_patch[part] : 0u, d1, d2); continue; }
        if (on) kf_note_on(0, (uint32_t)part, KF_PART_FIRST[part] + live_patch[part], d1, d2, 0, 4096);
        else kf_note_off(0, (uint32_t)part, d1);
    }
}

/* the keys: their edges, stamped with the frame being heard; from the main loop and while the screen is sent */
static uint32_t keys_prev;
static void keys_in(void)
{
    uint32_t keys = fm1_in.notes & 0x7FFFFFFu, down = keys & ~keys_prev, up = keys_prev & ~keys, k, now;
    if (!down && !up) return;
    keys_prev = keys;
    now = heard_now();
    for (k = 0; k < 27u; k++) {
        if (scr == SCR_PLAY) {
            if (paused || demo) continue;        /* paused: the keys rest; the DEMO plays itself */
            if (down >> k & 1u) kg_press(k, now);
            if (up >> k & 1u) kg_release(k, now);
        } else {                                 /* free play; in MIDI MODE they go out on channel 1 too */
            uint32_t pitch = (uint32_t)(53 + (int32_t)k + 12 * (scr == SCR_MIDI ? mm_oct : 0));
            if (down >> k & 1u) {
                kg_sound_on(k, pitch, 0, 4096);
                if (scr == SCR_MIDI) midi_send(0x90u, pitch, 100);
            }
            if (up >> k & 1u) {
                if (scr == SCR_MIDI && key_note[k]) midi_send(0x80u, key_note[k], 0);
                kg_sound_off(k, 0);
            }
        }
    }
}
static void app_idle(void)
{
    if (!idle_ok) return;
    keys_in();
    midi_in();
}

/* ---- a song */
static void voices_off(void)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) if (key_note[k]) kg_sound_off(k, 0);
}
static void song_start(uint32_t idx, uint32_t level, uint32_t is_demo)
{
    voices_off();
    demo = is_demo;
    kg_start(idx, level, is_demo ? 100u : set.speed, is_demo ? 0u : set.spread, kf_now + 6615u);   /* beat 0 in 150 ms */
    kg.auto_on = (uint8_t)(is_demo || autoplay);
    if (is_demo) kg_seek(kf_bench_beat);
    keys_prev = fm1_in.notes & 0x7FFFFFFu;       /* (a key held from before does not count) */
    paused = 0;
    scr = SCR_PLAY;
}
static void song_stop(void)
{
    kf_seq_stop();
    voices_off();
    paused = 0;
    demo = 0;
}
static void song_pause(void)
{
    if (!paused) {
        kf_ev_t e = {0, KF_EV_ALLOFF, 0, 0, 0, 0, 0, 0};
        voices_off();
        kf_sched_clear();
        kf_post(&e);
        paused = 1;
        paused_at = heard_now();
    } else {                                     /* the song goes on where it stopped: the band from there */
        const kf_song_t *s = kg.song;
        kf_seq.t0 += heard_now() - paused_at;
        kf_seq.bi = 0;
        while (kf_seq.bi < s->nband && (int32_t)s->band[kf_seq.bi].t * 256 < kg.pos) {
            const kf_bev_t *e = &s->band[kf_seq.bi++];
            if (e->ks) kf_seq.patch[e->part] = (uint8_t)(e->ks - 1u);
        }
        paused = 0;
    }
}

/* ---- the LEDs, as the reference's ledStates: a key lit when its note is at the line (or held), glowing when it
 * comes within 2 beats; OCT- OCT+ for a shift; PLAY on the beat; FX in a boost, blinking when the OPs are full */
static void leds(void)
{
    uint8_t L[27] = {0};
    uint32_t k, i;
    int32_t oct_m = 0, oct_p = 0, play = 0, fx = 0;
    if (scr == SCR_PLAY && !paused) {
        const kf_song_t *s = kg.song;
        int32_t pos = kg.pos, gb = (int32_t)((uint32_t)KG_BEAT * 11u * (s->bpm_x100 / 100u) / 6000u);   /* GOOD's window */
        for (i = kg.first; i < kg.n; i++) {
            const kf_cnote_t *n = &kg.notes[i];
            int32_t d = (int32_t)n->t * 256 - pos;
            uint32_t key = KG_LAYOUT[kg.spread ? 1 : 0][n->lane];
            if (d > 2 * KG_BEAT) break;
            if (kg.nst[i] == KN_HELD) L[key] = 2;
            else if (kg.nst[i] == KN_PENDING) { if (d >= -gb && d <= KG_BEAT / 2) L[key] = 2; else if (d > KG_BEAT / 2 && L[key] < 1) L[key] = 1; }
        }
        for (i = 0; i < s->noct && i < KG_MAXO; i++) {
            int32_t d = (int32_t)s->octs[i].t * 256 - pos, v;
            if (kg.ost[i] != KO_PENDING || d > 2 * KG_BEAT || d < -KG_BEAT / 2) continue;
            v = d <= KG_BEAT / 2 ? 2 : 1;
            if (s->octs[i].dir > 0) { if (v > oct_p) oct_p = v; } else if (v > oct_m) oct_m = v;
        }
        play = pos >= 0 && (pos / (KG_BEAT / 4)) % 4 == 0 ? 2 : 0;
        fx = kg_boosting() ? 2 : kg.ops >= 6 ? ((fm1_ms / 125u) % 2u ? 2 : 0) : 0;
    } else if (scr == SCR_SONGS) {
        for (i = 0; i < 8u; i++) L[KG_LAYOUT[set.spread ? 1 : 0][i]] = 1;   /* the landmarks: the layout's keys */
    }
    if (scr != SCR_PLAY) for (k = 0; k < 27u; k++) if (key_note[k]) L[k] = 2;
    for (k = 0; k < 27u; k++) { fm1_led_key(KEY_LED(k), L[k] == 2u); fm1_led_key_glow(KEY_LED(k), L[k] == 1u); }
    fm1_led_key(BTN_OCT_DOWN, oct_m == 2); fm1_led_key_glow(BTN_OCT_DOWN, oct_m == 1);
    fm1_led_key(BTN_OCT_UP, oct_p == 2); fm1_led_key_glow(BTN_OCT_UP, oct_p == 1);
    fm1_led_key(BTN_PLAY, play == 2 || (scr == SCR_SPLASH && (fm1_ms / 500u) % 2u == 0u));
    fm1_led_key(BTN_FX, fx == 2);
    fm1_led_key(BTN_GLO, set.spread != 0u);
}

/* ---- MIDI MODE LIVE's screen: the channels, their patches, the clock */
static void draw_midi_mode(void)
{
    static const uint8_t CHS[7] = {0, 1, 2, 3, 4, 5, 9};
    static const char *const KM[4] = {"MODE", "TONE", "BPM", "MIX"};
    char s[32], *p;
    uint32_t i;
    krect(0, 0, 240, 240, K_BG);
    ks_header(0, "MIDI MODE", "LIVE", -1, K_CYAN);
    if (clk_bpm_x100 && fm1_ms - clk_t0 < 3000u) { p = knum(s, (clk_bpm_x100 + 50u) / 100u, 0); kput(p, " BPM"); ktext(&FONT_S, 4, 21, s, K_GREEN); }
    else ktext(&FONT_S, 4, 21, "NO CLOCK", K_DIM);
    ktextR(&FONT_S, 236, 21, midi_seen ? "MIDI IN" : "DAW > FM-1", K_DIM);
    for (i = 0; i < 7u; i++) {
        uint32_t ch = CHS[i], part = (uint32_t)CH_PART[ch], act = ch_act[ch] && fm1_ms - ch_act[ch] < 150u;
        int32_t y = 44 + (int32_t)i * 22;
        const char *pn = part == KF_P_DRUMS ? KF_KIT[live_patch[part] < KF_NKIT ? live_patch[part] : 0u].name :
                         part == KF_P_PLAY ? tone_name() : KF_PATCH[KF_PART_FIRST[part] + live_patch[part]].name;
        p = kput(s, "CH "); knum(p, ch + 1u, 0);
        krect(2, y - 2, 236, 20, act ? K_SURF2 : K_SURF);
        krect(2, y - 2, 3, 20, act ? (i ? K_VIOLET : K_CYAN) : K_LINE);
        ktext(&FONT_S, 9, y, s, i ? K_VIOLET : K_CYAN);
        ktext(&FONT_S, 60, y, PART_NAME7[part], K_TEXT);
        ktextR(&FONT_S, 234, y, pn, act ? K_TEXT : K_DIM);
    }
    ktextC(&FONT_S, 120, 200, "KEYS OUT ON CH 1", K_MUTE);
    ks_knobs(KM, 5u);
}

static void draw(void)
{
    static const char VER[] = "v" APP_VER;
    ks_menu_t m;
    uint32_t i;
    char num[4];
    idle_ok = 1;                                 /* (cv_begin waits for the last frame's DMA: keys and MIDI meanwhile) */
    cv_begin(240, 240, c565(K_BG));
    idle_ok = 0;
    knum(num, (scr == SCR_PLAY || scr == SCR_RESULTS ? kg.idx : set.sel) + 1u, 2);
    if (scr == SCR_SPLASH) ks_splash(fm1_ms, VER);
    else if (scr == SCR_SONGS) {
        m.sel = set.sel; m.level = set.level; m.speed = set.speed; m.spread = set.spread; m.tone = set.tone; m.mm_mode = 0;
        for (i = 0; i < KF_NSONG && i < 8u; i++) { m.best[i][0] = best_of(i, 0); m.best[i][1] = best_of(i, 1); }
        ks_songs(&m, tone_name());
    } else if (scr == SCR_PLAY) {
        uint8_t lit[8], led[8];
        for (i = 0; i < 8u; i++) {
            uint32_t key = KG_LAYOUT[kg.spread ? 1 : 0][i];
            lit[i] = (uint8_t)(key_note[key] != 0u);
            led[i] = (uint8_t)(lit[i] ? 2u : 0u);
        }
        ks_play(fm1_ms, lit, led, num, demo ? "DEMO" : kg.auto_on ? "AUTO" : 0);
        if (paused) ks_banner("PAUSE", "PLAY GOES ON, HOME QUITS", K_AMBER, 92);
    } else if (scr == SCR_RESULTS) ks_results(num);
    else draw_midi_mode();
    if (toast_ms && fm1_ms - toast_ms < 900u) ks_toast(toast_s);
    else toast_ms = 0;
    cv_blit(0, 0);                               /* (the DMA goes on while the loop does) */
}

static int32_t enc_step(uint32_t enc, int32_t v, int32_t by, int32_t lo, int32_t hi, uint32_t *changed)
{
    int32_t e = fm1_enc_take(enc);
    if (!e) return v;
    v += e * by;
    v = v < lo ? lo : v > hi ? hi : v;
    *changed = 1;
    return v;
}
static void toast_num(const char *label, uint32_t v, const char *unit)
{
    char s[24], *p = kput(s, label);
    p = knum(p, v, 0);
    kput(p, unit);
    toast(s);
}

static void app_main(void)
{
    uint32_t last_draw = 0;
    if (settings_load(&set, sizeof set) || set.level > 1u || set.speed < 50u || set.speed > 100u || set.tone > 5u ||
        set.mix > 100u || set.grit > 100u || set.sel > KF_NSONG)
        set_defaults();
    offset_ms = set.offset;
    kf_init();
    kf_set_grit(set.grit);
    kf_set_mix(set.mix);
    kh_init();
    scr = SCR_SPLASH;

    for (;;) {
        uint32_t b, rel, now_ms, ch = 0;
        int32_t e, v;
        platform_poll();
        keys_in();
        midi_in();
        now_ms = fm1_ms;

        b = fm1_input_edges(&rel);
        if (scr == SCR_SPLASH) {
            if (b & (1u << BTN_PLAY)) { scr = SCR_SONGS; redraw = 1; }
            else if (b & (1u << BTN_FX)) song_start(KF_BENCH_SONG, 1, 1);
        } else if (scr == SCR_SONGS) {
            if (b & (1u << BTN_PLAY)) {
                if (set.sel == KF_NSONG) { scr = SCR_MIDI; mm_oct = 0; kf_set_tempo(clk_bpm_x100 ? clk_bpm_x100 : 12000u); }
                else song_start(set.sel, set.level, 0);
                redraw = 1;
            }
            if (b & (1u << BTN_HOME)) { scr = SCR_SPLASH; redraw = 1; }
            if (b & (1u << BTN_GLO)) { set.spread ^= 1u; toast(set.spread ? "SPREAD ON" : "SPREAD OFF"); ch = 1; }
            if (b & (1u << BTN_SEQ)) { autoplay ^= 1u; toast(autoplay ? "AUTO ON" : "AUTO OFF"); }
            v = enc_step(ENC_ALGORITHM, (int32_t)set.grit, 10, 0, 100, &ch);
            if (v != (int32_t)set.grit) { set.grit = (uint8_t)v; kf_set_grit(set.grit); toast_num("GRIT ", (uint32_t)v, ""); }
            v = enc_step(ENC_SELECT, (int32_t)set.sel, 1, 0, (int32_t)KF_NSONG, &ch);
            if (v != (int32_t)set.sel) { set.sel = (uint8_t)v; redraw = 1; }
            v = enc_step(ENC_KNOB1, (int32_t)set.level, 1, 0, 1, &ch);
            if (v != (int32_t)set.level) { set.level = (uint8_t)v; toast(v ? "LEVEL MED" : "LEVEL EASY"); }
            if ((e = fm1_enc_take(ENC_KNOB2)) != 0) { set.tone = (uint8_t)((set.tone + 6u + (uint32_t)(e % 6 + 6)) % 6u); toast(tone_name()); ch = 1; }
            v = enc_step(ENC_KNOB3, (int32_t)set.speed, 10, 50, 100, &ch);
            if (v != (int32_t)set.speed) { set.speed = (uint8_t)v; toast_num("SPEED ", (uint32_t)v, "%"); }
        } else if (scr == SCR_PLAY) {
            if (b & (1u << BTN_HOME) || (demo && b & (1u << BTN_PLAY))) { int d = (int)demo; song_stop(); scr = d ? SCR_SPLASH : SCR_SONGS; redraw = 1; }
            else if (b & (1u << BTN_PLAY) && !kg.done) song_pause();
            else if (!paused) {
                if (!demo) {
                    uint32_t frame = heard_now();
                    if (b & (1u << BTN_FX)) kg_boost();
                    if (b & (1u << BTN_OCT_DOWN)) kg_oct_press(-1, frame);
                    if (b & (1u << BTN_OCT_UP)) kg_oct_press(1, frame);
                }
                kf_seq_post(kf_now + 4096u);   /* the band, 93 ms ahead of the speaker */
                if (kg_update(heard_now())) {
                    if (demo) song_start(KF_BENCH_SONG, 1, 1);   /* the DEMO goes round */
                    else {
                        kg.new_best = kg.auto_on ? 0 : best_put(kg.idx, kg.level, kg.acc10);   /* (AUTO: not kept) */
                        ch = 1;
                        song_stop();
                        scr = SCR_RESULTS;
                        redraw = 1;
                    }
                }
            }
        } else if (scr == SCR_RESULTS) {
            if (b & (1u << BTN_PLAY)) song_start(kg.idx, kg.level, 0);
            if (b & (1u << BTN_HOME)) { scr = SCR_SONGS; redraw = 1; }
        } else if (scr == SCR_MIDI) {
            if (b & (1u << BTN_HOME)) {
                kf_ev_t ev = {0, KF_EV_ALLOFF, 0, 0, 0, 0, 0, 0};
                voices_off();
                kf_post(&ev);
                scr = SCR_SONGS; redraw = 1;
            }
            if (b & (1u << BTN_OCT_DOWN) && mm_oct > -2) mm_oct--;
            if (b & (1u << BTN_OCT_UP) && mm_oct < 2) mm_oct++;
            if ((e = fm1_enc_take(ENC_KNOB2)) != 0) { set.tone = (uint8_t)((set.tone + 6u + (uint32_t)(e % 6 + 6)) % 6u); toast(tone_name()); ch = 1; }
        }
        if (scr != SCR_PLAY) {                   /* KNOB 4: MIX, everywhere but in a song */
            v = enc_step(ENC_KNOB4, (int32_t)set.mix, 10, 0, 100, &ch);
            if (v != (int32_t)set.mix) { set.mix = (uint8_t)v; kf_set_mix(set.mix); toast_num("MIX ", (uint32_t)v, ""); }
        }
        if (ch) { dirty_set = 1; t_set = now_ms; }

        leds();
        if (scr == SCR_PLAY || redraw || ((scr == SCR_SPLASH || scr == SCR_MIDI || toast_ms) && now_ms - last_draw >= 33u)) {
            kg_ms = now_ms;                      /* the play as fast as it goes, the rest at 30 fps when it moves */
            draw();
            redraw = 0;
            last_draw = now_ms;
        }

        if (dirty_set && scr != SCR_PLAY && now_ms - t_set > 2000u) {   /* when silent: an erase stalls the sound */
            uint32_t k, quiet = 1;
            for (k = 0; k < KF_NV; k++) if (kf_v[k].state) quiet = 0;
            if (quiet) { settings_save(&set, sizeof set); dirty_set = 0; }
        }
    }
}
