/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk) */
/* KEYFALL's rules on the host (firmware/src/kf_game.h), as tests/reference.cjs checks the reference's: every song
 * on autoplay, EASY and MED, normal and SPREAD: 100 %, no miss, a full combo, every OCT shift, OP BOOST where the
 * song has phrases; a press 80 ms late is GOOD; a wrong key breaks the combo; a key outside the layout does nothing
 * (D1, D2); a hold let go at half is broken. */
#include <stdio.h>
#include <string.h>
#define KF_POOL
#include "kf_engine.h"
#include "kf_game.h"

static int fails;
static void check(int ok, const char *what) { printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) fails++; }
static uint32_t sounds;
static void kg_sound_on(uint32_t key, uint32_t pitch, int32_t detune, uint32_t bright) { (void)key; (void)pitch; (void)detune; (void)bright; sounds++; }
static void kg_sound_off(uint32_t key, uint32_t pitch) { (void)key; (void)pitch; }

static void run_song(uint32_t idx, uint32_t level, uint32_t spread)
{
    uint32_t f, boosts = 0, was = 0;
    char what[200];
    kf_init();
    kg_start(idx, level, 100, spread, 44100);
    kg.auto_on = 1;
    for (f = 0; f < 44100u * 400u; f += 256u) {   /* the main loop's passes, a block apart */
        kg_ms = f / 44;
        if (kg_update(f)) break;
        if (kg_boosting() && !was) boosts++;
        was = (uint32_t)kg_boosting();
    }
    snprintf(what, sizeof what, "%-15s %s%s: %d.%d %% (%c), combo %d of %d, OCT %d of %u, %u boost(s), score %d",
             kg.song->title, level ? "MED " : "EASY", spread ? " SPREAD" : "", kg.acc10 / 10, kg.acc10 % 10, kg.grade,
             kg.max_combo, kg.total, kg.cnt[4], kg.song->noct, boosts, kg.score);
    check(kg.done && kg.acc10 == 1000 && kg.cnt[3] == 0 && kg.full && kg.cnt[4] == kg.song->noct && (boosts > 0 || !kg.song->nphrase), what);
}

int main(void)
{
    uint32_t s, l, i, n0;
    for (s = 0; s < KF_NSONG; s++)
        for (l = 0; l < 2u; l++) run_song(s, l, 0);
    run_song(1, 1, 1);                           /* NEON CARRIER with SPREAD (D2) */

    /* by hand: NEON CARRIER MED, the first note */
    kf_init();
    kg_start(1, 1, 100, 0, 44100);
    n0 = kg_note_frame(0);
    kg_update(n0 - 44100u);
    {
        uint32_t lane = kg.notes[0].lane, key = KG_LAYOUT[0][lane];
        int32_t oct_need = kg.notes[0].oct;
        kg.oct = oct_need;
        kg_press(key, n0 + 3528u);                /* 80 ms late */
        check(kg.cnt[2] == 1 && kg.judge == KJ_GOOD, "a press 80 ms late is GOOD");
        kg_release(key, n0 + 4000u);
    }
    {
        uint32_t c = (uint32_t)kg.combo, n1 = kg_note_frame(1), lane = kg.notes[1].lane, key = KG_LAYOUT[0][(lane + 1u) % 8u];
        kg_update(n1 - 100u);
        kg_press(key, n1);
        check(c == 1u && kg.combo == 0 && kg.judge == KJ_WRONG, "the wrong key breaks the combo (WRONG)");
        kg_release(key, n1 + 100u);
    }
    {
        int32_t sc = kg.score, cb = kg.combo;
        uint32_t n2 = kg_note_frame(2);
        kg_press(8, n2);                          /* F#4: a black key */
        kg_press(10, n2);                         /* G#4 */
        kg_press(0, n2);                          /* F3: white, outside the layout */
        check(kg.score == sc && kg.combo == cb, "a black key, or a white one outside the layout, does nothing (D1)");
    }
    /* a hold let go at half: find one */
    for (s = 0; s < KF_NSONG; s++) {
        const kf_song_t *so = &KF_SONG[s];
        for (i = 0; i < so->nchart[1]; i++) if (so->chart[1][i].flags & 1u) break;
        if (i < so->nchart[1]) break;
    }
    if (s < KF_NSONG) {
        uint32_t nf, key;
        kf_init();
        kg_start(s, 1, 100, 0, 44100);
        nf = kg_note_frame(i);
        kg_update(nf - 22050u);
        kg.oct = kg.notes[i].oct;
        key = KG_LAYOUT[0][kg.notes[i].lane];
        kg_press(key, nf);
        check(kg.nst[i] == KN_HELD, "a hold is held");
        kg_release(key, nf + kf_seq_len(kg.notes[i].len) / 2u);
        check(kg.nst[i] == KN_BROKEN, "a hold let go at half is broken");
    } else check(0, "no song with a hold");
    printf(fails ? "kf_game_test: %d FAILED\n" : "kf_game_test: all passed\n", fails);
    return fails ? 1 : 0;
}
