/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Joakim (joakimskurk) */
/* The engine's worst case, to measure it (tests/kf/kf_render.c stress; the firmware's DEMO, which fm1-emu times):
 * ENGINE TOUR's finale as docs/fm1-budsjett.md counts it. Every part on its gritty patch at once: FUZZ (the
 * player, bright), DIRT BASS in 8ths (overlapping), GRIT PAD (3 notes), CRUSH arpeggio in 16ths, FUZZ LEAD, GRIT EP
 * stabs (3 notes), the DIRT kit with 16th hats; 120 BPM, 4 bars, looped. */
#pragma once
#include "kf_engine.h"

static uint8_t kf_bench_pid(const char *name)
{
    uint32_t i, k;
    for (i = 0; i < KF_NPATCH; i++) {
        const char *a = KF_PATCH[i].name;
        for (k = 0; a[k] && a[k] == name[k]; k++) {}
        if (!a[k] && !name[k]) return (uint8_t)i;
    }
    return 0;
}
static uint32_t kf_bench_step;                   /* the next 16th to post */
static uint32_t kf_bench_t0;                     /* its bar 1's frame */
static void kf_bench_start(uint32_t t0) { kf_bench_step = 0; kf_bench_t0 = t0; kf_sched_clear(); kf_set_tempo(12000); }
static void kf_bench_post(uint32_t upto)         /* post the 16ths that start before upto (frames) */
{
    static const uint8_t CH[4][3] = {{57, 60, 64}, {53, 57, 60}, {55, 59, 62}, {52, 55, 59}};
    static uint8_t fuzz, dirt, gpad, crush, flead, gep, ok;
    const uint32_t S16 = 5513u, SPB = 22050u;    /* frames: a 16th, a beat (120 BPM) */
    if (!ok) {
        fuzz = kf_bench_pid("FUZZ"); dirt = kf_bench_pid("DIRT BASS"); gpad = kf_bench_pid("GRIT PAD");
        crush = kf_bench_pid("CRUSH"); flead = kf_bench_pid("FUZZ LEAD"); gep = kf_bench_pid("GRIT EP"); ok = 1;
    }
    for (;;) {
        uint32_t st = kf_bench_step % 64u, t = kf_bench_t0 + kf_bench_step * 44100u / 8u, j;
        const uint8_t *c = CH[st / 16u];
        if ((int32_t)(t - upto) >= 0) break;
        if (st % 16u == 0u) {                    /* a bar: the pad, the keys, the lead */
            for (j = 0; j < 3u; j++) kf_sched_note(t, KF_P_PAD, gpad, c[j], 90, 4u * SPB * 98u / 100u);
            for (j = 0; j < 3u; j++) kf_sched_note(t, KF_P_KEYS, gep, c[j] + 12u, 90, SPB * 3u / 2u);
            kf_sched_note(t, KF_P_LEAD, flead, c[2] + 12u, 100, 2u * SPB);
        }
        kf_sched_note(t, KF_P_ARP, crush, c[(st % 16u) % 3u] + 12u, 90, S16 * 98u / 100u);
        if (st % 2u == 0u) kf_sched_note(t, KF_P_BASS, dirt, c[0] - 24u, 100, S16 * 22u / 10u);
        kf_drum(t, 3, 42, 80);
        if (st % 4u == 0u) kf_drum(t, 3, 36, 110);
        if (st % 8u == 4u) kf_drum(t, 3, 38, 105);
        if (st % 4u == 2u) { kf_sched_flush(t + 1u); kf_note_on(t, KF_P_PLAY, fuzz, c[1] + 24u, 104, 0, 7000);
                             kf_offs[kf_noffs].t = t + SPB * 9u / 10u; kf_offs[kf_noffs].part = KF_P_PLAY;
                             kf_offs[kf_noffs].pitch = (uint8_t)(c[1] + 24u); kf_noffs++; }
        kf_bench_step++;
    }
    kf_sched_flush(upto);
}
