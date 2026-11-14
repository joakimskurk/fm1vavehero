/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk) */
/* VaveHero's rules on the host (firmware/src/kf_game.h), as tests/reference.cjs checks the reference's: every song
 * on autoplay at its own level (D12), normal and SPREAD: 100 %, no miss, a full combo, every OCT shift, OP BOOST where
 * the song has phrases; a press 80 ms late is GOOD; a wrong key breaks the combo; a key outside the layout does nothing
 * (D1, D2); EASY takes the right note in any octave, MED does not; in a FREE SOLO every key plays and nothing is judged
 * (D14); a hold let go at half is broken. */
#include <stdio.h>
#include <string.h>
#define KF_POOL
#include "kf_engine.h"
#include "kf_game.h"

static int fails;
static void check(int ok, const char *what) { printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) fails++; }
static uint32_t sounds;
static uint8_t sounding[27];                      /* a key's voice on: autoplay must let go of every key it presses */
static void kg_sound_on(uint32_t key, uint32_t pitch, int32_t detune, uint32_t bright) { (void)pitch; (void)detune; (void)bright; sounds++; if (key < 27u) sounding[key] = 1; }
static void kg_sound_off(uint32_t key, uint32_t pitch) { (void)pitch; if (key < 27u) sounding[key] = 0; }

static const char *const LV[4] = {"EASY", "MED", "HARD", "EXP"};

static void run_song(uint32_t idx, uint32_t spread)
{
    uint32_t f, boosts = 0, was = 0, k, left = 0;
    char what[200];
    memset(sounding, 0, sizeof sounding);
    kf_init();
    kg_start(idx, 100, spread, 44100);
    kg_count_in(KG_COUNT_IN);                    /* (as the app starts a song: D15) */
    kg.auto_on = 1;
    for (f = 0; f < 44100u * 400u; f += 256u) {   /* the main loop's passes, a block apart */
        kg_ms = f / 44;
        if (kg_update(f)) break;
        if (kg_boosting() && !was) boosts++;
        was = (uint32_t)kg_boosting();
    }
    snprintf(what, sizeof what, "%-15s %-4s%s: %d.%d %% (%c), combo %d of %d, OCT %d of %u, %u boost(s), score %d",
             kg.song->title, LV[kg.level & 3u], spread ? " SPREAD" : "", kg.acc10 / 10, kg.acc10 % 10, kg.grade,
             kg.max_combo, kg.total, kg.cnt[4], kg.song->noct, boosts, kg.score);
    check(kg.done && kg.acc10 == 1000 && kg.cnt[3] == 0 && kg.full && kg.cnt[4] == kg.song->noct && (boosts > 0 || !kg.song->nphrase), what);
    for (k = 0; k < 27u; k++) left += sounding[k];
    snprintf(what, sizeof what, "%-15s autoplay lets go of every key it pressed (%u left sounding)", kg.song->title, left);
    check(left == 0, what);
}

int main(void)
{
    uint32_t s, i, n0;
    char lv[80], *p = lv;
    for (s = 0; s < KF_NSONG; s++) {
        run_song(s, 0);
        p += snprintf(p, (size_t)(lv + sizeof lv - p), "%s%s", s ? "," : "", LV[KF_SONG[s].level & 3u]);
    }
    check(!strcmp(lv, "EASY,MED,HARD,MED,EXP,EXP,HARD,HARD"), "D12: each song has its one level (FIRST LIGHT EASY, NEON CARRIER MED, ENGINE TOUR HARD, CHROME HORIZON MED, KIRAMEKI RUSH EXP, YOZORA SPARK EXP, TURBO NOIR HARD, DISCO VOLTAGE HARD)");
    run_song(1, 1);                              /* NEON CARRIER with SPREAD (D2) */

    /* D15: the count-in. A bar before beat 0, a closed hi-hat on each beat in the song's first kit, and the first notes
     * on their way: KIRAMEKI RUSH's first note is on beat 0 */
    {
        uint32_t t0 = 44100, beat, hats = 0, ok = 1, kit = 99, first_band = 0xFFFFFFFFu;
        char what[200];
        kf_init();
        kg_start(4, 100, 0, t0);
        kg_count_in(KG_COUNT_IN);
        beat = kf_seq_len(96);
        kf_seq_post(t0 + 6u * beat);
        while (kf_qr[1] != kf_qw[1]) {
            const kf_ev_t *e = &kf_q[1][kf_qr[1] % KF_QN];
            if (e->type == KF_EV_DRUM && e->t < t0 + 4u * beat) {
                ok &= e->pitch == 42u && e->t == kf_seq.t0 - kf_seq_len((4u - hats) * 96u);
                kit = e->patch; hats++;
            } else if (e->t < first_band) first_band = e->t;
            kf_qr[1]++;
        }
        snprintf(what, sizeof what, "D15: %s starts with a bar of hi-hat: %u hats a beat apart in kit %u, beat 0 a bar later (%d beats now), the band from there, its first note at beat %u",
                 kg.song->title, hats, kit, kg.pos / KG_BEAT, kg.notes[0].t / 96u);
        check(hats == 4 && ok && kit < KF_NKIT && kg.pos + 4 * KG_BEAT >= -2 && kg.pos + 4 * KG_BEAT <= 2 && first_band >= kf_seq.t0 && kf_seq.t0 == t0 + kf_seq_len(4u * 96u) && kg.notes[0].t == 0, what);
        kf_init();
        kg_start(2, 100, 0, t0);                 /* the DEMO's way in: no count-in */
        kf_seq_post(t0 + 4u * beat);
        for (hats = 0; kf_qr[1] != kf_qw[1]; kf_qr[1]++) hats += kf_q[1][kf_qr[1] % KF_QN].t < t0;
        check(hats == 0 && kg.pos == 0, "D15: without kg_count_in a song starts on beat 0 (the DEMO), nothing before it");
    }

    /* EASY (FIRST LIGHT) takes the right note in any octave; MED (NEON CARRIER) does not */
    for (s = 0; s < 2u; s++) {
        uint32_t nf, key;
        kf_init();
        kg_start(s, 100, 0, 44100);
        nf = kg_note_frame(0);
        kg_update(nf - 22050u);
        kg.oct = kg.notes[0].oct + 1;            /* an octave off */
        key = KG_LAYOUT[0][kg.notes[0].lane];
        kg_press(key, nf);
        check(s == 0 ? kg.nst[0] != KN_PENDING && kg.judge == KJ_PERFECT : kg.judge == KJ_WRONG,
              s == 0 ? "EASY: the right note an octave off is PERFECT" : "MED: the right note an octave off is WRONG");
        kg_release(key, nf + 2000u);
    }

    /* by hand: NEON CARRIER (MED), the first note */
    kf_init();
    kg_start(1, 100, 0, 44100);
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
    /* FREE SOLO (D14): CHROME HORIZON's, over its breakdown. Every key sounds and nothing is judged; the note at its end
     * is judged as ever, even pressed early, inside the solo */
    {
        const kf_song_t *so = &KF_SONG[3];
        check(so->nsolo == 1 && so->solos[0].a == 96 * 96 && so->solos[0].b == 112 * 96, "D14: CHROME HORIZON has its FREE SOLO, bars 25-28");
        if (so->nsolo) {
            uint32_t f, s0, at;
            int32_t sc, cb, j;
            kf_init();
            kg_start(3, 100, 0, 44100);
            kg.auto_on = 1;
            for (f = 0; kg.pos < (int32_t)so->solos[0].a * 256 + KG_BEAT; f += 256u) { kg_ms = f / 44; kg_update(f); }
            kg.auto_on = 0;
            sc = kg.score; cb = kg.combo; s0 = sounds;
            check(kg_solo_at(kg.pos) == 0 && cb > 0, "the song reaches its solo with a combo");
            kg_press(8, f); kg_release(8, f + 2000u);       /* C#4: a black key */
            kg_press(0, f); kg_release(0, f + 2000u);       /* F3: a white key outside the layout */
            kg_press(KG_LAYOUT[0][3], f); kg_release(KG_LAYOUT[0][3], f + 2000u);
            check(sounds == s0 + 3u && kg.score == sc && kg.combo == cb && kg.judge != KJ_WRONG,
                  "in a FREE SOLO every key sounds, black ones too, and nothing is judged: no score, no WRONG, the combo kept");
            for (j = (int32_t)kg.first; j < (int32_t)kg.n && kg.notes[j].t < so->solos[0].b; j++) {}
            at = kg_note_frame((uint32_t)j) - 2000u;        /* the note at the solo's end, 45 ms early: inside the solo */
            for (; f + 256u < at; f += 256u) { kg_ms = f / 44; kg_update(f); }
            kg.oct = kg.notes[j].oct;
            check(kg_solo_at(kg_pos_at(at)) == 0, "(the early press is inside the solo)");
            kg_press(KG_LAYOUT[0][kg.notes[j].lane], at);
            check(kg.nst[j] != KN_PENDING && kg.judge == KJ_GREAT, "the note at a FREE SOLO's end is judged, pressed early inside the solo: GREAT");
        }
    }

    /* a MIDI keyboard's note just after an OCT shift, 80 ms early: judged in the octave it was mapped to a key in (the
     * song's), not in the player's, which follows only 220 ms after the shift: GOOD, not WRONG */
    {
        const kf_song_t *so = &KF_SONG[1];
        uint32_t j, f = 0, at;
        int32_t oct;
        kf_init();
        kg_start(1, 100, 0, 44100);
        kg.midi_used = 1;
        for (j = 0; j < kg.n && kg.notes[j].t <= so->octs[0].t; j++) {}
        at = kg_note_frame(j) - 3528u;
        for (; f + 256u < at; f += 256u) { kg_ms = f / 44; kg_update(f); }
        oct = kg_oct_at(kg.pos);
        kg_press_oct((uint32_t)(kg.notes[j].pitch - 53 - 12 * oct), at, oct);
        check(oct != kg.oct && kg.nst[j] != KN_PENDING && kg.judge == KJ_GOOD, "MIDI just after a shift: judged in the song's octave (GOOD), not the player's (WRONG)");
    }
    /* autoplay: a note's release does not cut the next note on the same key (a quarter that overlaps a hold a little) */
    {
        static const kf_cnote_t N2[2] = {{96 * 8, 96 * 21 / 20, 60, 0, 0, 0}, {96 * 8 + 24, 192, 60, 0, 0, 1}};
        uint32_t f;
        kf_init();
        kg_start(1, 100, 0, 44100);
        kg.notes = N2; kg.n = 2; kg.nst[0] = kg.nst[1] = KN_PENDING; kg.adn[0] = kg.adn[1] = 0;
        kg.auto_on = 1;
        for (f = 44100; f < 44100u * 8u; f += 256u) { kg_ms = f / 44; kg_update(f); }
        check(kg.nst[0] == KN_HIT && kg.nst[1] == KN_DONE, "autoplay: the first note's release does not break the hold after it on the same key");
    }

    /* a hold let go at half: find one */
    for (s = 0; s < KF_NSONG; s++) {
        const kf_song_t *so = &KF_SONG[s];
        if (so->level == 0) continue;            /* (on MED and up: EASY takes any octave) */
        for (i = 0; i < so->nchart; i++) if (so->chart[i].flags & 1u) break;
        if (i < so->nchart) break;
    }
    if (s < KF_NSONG) {
        uint32_t nf, key;
        kf_init();
        kg_start(s, 100, 0, 44100);
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
