#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Joakim (joakimskurk)
"""The built-in songs, written as standard MIDI files exactly as docs/songs.md asks of a song made in a DAW, and
embedded in reference/vavehero.html so the reference stays one file. The reference imports them with the same code
as a file you load yourself.

    python3 tools/songs.py            # writes songs/*.mid and the songs block in reference/vavehero.html

Channels (1-based, as a DAW shows them): 1 PLAY (the play track), 2 BASS, 3 PAD, 4 ARP, 5 LEAD, 6 KEYS,
10 DRUMS (General MIDI notes). Channel 16 belongs to LINK. Everything is a note, no CC and no program change:
notes 0-11 (C-1..B-1) on a channel are keyswitches that pick its patch, note 0 the first (D6), and note 11 (B-1)
on channel 1, held, marks an OP phrase.
"""
import base64
import json
import os
import re
import struct

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PPQ = 480
CH = dict(PLAY=0, BASS=1, PAD=2, ARP=3, LEAD=4, KEYS=5, DRUMS=9)       # 0-based on the wire
NAMES = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}
CHORD = {'C': [0, 4, 7], 'G': [7, 11, 14], 'Am': [9, 12, 16], 'F': [5, 9, 12], 'Em': [4, 7, 11], 'Dm': [2, 5, 9],
         'Bb': [10, 14, 17], 'Gm': [7, 10, 14], 'E': [4, 8, 11], 'A': [9, 13, 16], 'D': [2, 6, 9], 'F#m': [6, 9, 13],
         'Bm': [11, 14, 18]}
ROOT = {'C': 0, 'G': 7, 'Am': 9, 'F': 5, 'Em': 4, 'Dm': 2, 'Bb': 10, 'Gm': 7, 'E': 4, 'A': 9, 'D': 2, 'F#m': 6, 'Bm': 11}
KICK, SNARE, CLAP, HAT, OHAT, CRASH, TOM_LO, TOM_MID, TOM_HI = 36, 38, 39, 42, 46, 49, 45, 48, 50
OP_NOTE = 11                                                            # B-1 on channel 1, held over an OP phrase
SOLO_NOTE = 10                                                          # A#-1 on channel 1, held over a FREE SOLO (D14)


def pitch(name):
    m = re.fullmatch(r'([A-G]#?)(\d)', name)
    return 12 * (int(m.group(2)) + 1) + NAMES[m.group(1)]


def melody(src, vel=100):
    """'bar: note beat length | ...', chords as C4+E4 -> [(beat, pitch, length, velocity)]"""
    out = []
    for line in src.strip().splitlines():
        bar, rest = line.split(':')
        for part in rest.split('|'):
            n, at, ln = part.split()
            for nn in n.split('+'):
                out.append((int(bar) * 4 + float(at), pitch(nn), float(ln), vel))
    return out


def segments(chords, bar):
    segs = chords[bar].split()
    return [(bar * 4 + i * 4 / len(segs), ch, 4 / len(segs)) for i, ch in enumerate(segs)]


def bass_root(ch):
    return 40 + (ROOT[ch] - 4) % 12                                     # E2..D#3


def voicing(ch, base=52):
    """the chord's tones folded into one octave from base (E3..D#4 by default)"""
    return sorted(base + (iv - base) % 12 for iv in CHORD[ch])


# ---------------------------------------------------------------- the songs

FIRST_LIGHT = dict(
    file='first-light.mid', title='FIRST LIGHT', bpm=96,
    chords=['C', 'G', 'C', 'G', 'Am', 'F', 'C', 'G', 'Am', 'F', 'C', 'G', 'C', 'C', 'C'],
    phrases=[(3, 4), (7, 8), (10, 11)],
    play='''
2: C4 0 1 | E4 1 1 | G4 2 2
3: G4 0 1 | F4 1 1 | E4 2 1 | D4 3 1
4: E4 0 1 | E4 1 1 | C4 2 2
5: F4 0 1 | E4 1 1 | D4 2 2
6: C4 0 1 | E4 1 1 | G4 2 1 | E4 3 1
7: D4 0 2 | G4 2 2
8: E4 0 1 | F4 1 1 | E4 2 1 | C4 3 1
9: D4 0 1 | E4 1 1 | F4 2 2
10: E4 0 1 | D4 1 1 | C4 2 1 | E4 3 1
11: D4 0 3
12: C4+E4 0 1 | E4+G4 2 2
13: C4+E4+G4 0 3''')

NEON_CARRIER = dict(
    file='neon-carrier.mid', title='NEON CARRIER', bpm=112,
    chords=['Am', 'G', 'Am', 'F', 'C', 'G', 'Am', 'F', 'C', 'G', 'F', 'G', 'Em', 'Am', 'F', 'G', 'Em', 'Am',
            'C', 'G', 'Am', 'F', 'C', 'G', 'Am', 'F G', 'C', 'C'],
    phrases=[(2, 3), (6, 7), (10, 11), (14, 15), (18, 19), (22, 23)],
    play='''
2: E4 0 1 | A4 1 1 | C5 2 1.5 | B4 3.5 .5
3: A4 0 1 | G4 1 .5 | F4 1.5 .5 | E4 2 2
4: E4 0 .5 | G4 .5 .5 | C5 1 1 | B4 2 .5 | A4 2.5 .5 | G4 3 1
5: D4 0 1 | G4 1 1 | B4 2 2
6: E4 0 1 | A4 1 1 | C5 2 1 | E4 3 .5 | D4 3.5 .5
7: E4 0 .5 | F4 .5 .5 | A4 1 1 | G4 2 1 | F4 3 1
8: E4 0 1 | C4 1 1 | G4 2 2
9: D4+G4 0 2 | B4 2 1 | A4 3 .5
10: F5 0 1 | A5 1 1 | C6 2 1 | A5 3 1
11: G5 0 1 | B5 1 .5 | A5 1.5 .5 | G5 2 2
12: E5 0 .5 | G5 .5 .5 | B5 1 1 | G5 2 .5 | E5 2.5 .5 | D5 3 1
13: E5 0 1 | A5 1 1 | A5+C6 2 2
14: F5 0 .5 | G5 .5 .5 | A5 1 .5 | C6 1.5 1.5 | A5 3 1
15: B5 0 1 | G5 1 1 | D5 2 1 | G5 3 1
16: G5 0 1 | D5 1 1 | E5 2 2
17: A5 0 1 | C6 1 1 | B5 2 1 | G5 3 .5
18: E4 0 1 | G4 1 .5 | C5 1.5 1.5 | B4 3 .5 | A4 3.5 .5
19: G4 0 2 | D4 2 1 | G4 3 1
20: A4 0 1 | C5 1 .5 | B4 1.5 .5 | A4 2 1 | E4 3 1
21: F4 0 .5 | G4 .5 .5 | A4 1 1 | F4+A4 2 2
22: E4 0 1 | G4 1 .5 | C5 1.5 1.5 | B4 3 .5 | C5 3.5 .5
23: B4 0 1 | A4 1 .5 | G4 1.5 .5 | D4 2 1 | G4 3 1
24: A4+C5 0 1 | G4+B4 1 1 | E4+A4 2 2
25: F4 0 1 | E4 1 1 | D4 2 1 | D4+G4 3 1
26: C4+E4+G4 0 3''',
    lead='''
0: E5 0 1 | A5 1 1 | C6 2 1.5 | B5 3.5 .5
1: A5 0 1 | G5 1 .5 | F5 1.5 .5 | E5 2 2''')


def arrange_first_light(s):
    """a sunrise: pads from the first bar, a soft pulse, an arpeggio from bar 6, a gentle gated snare"""
    bass, pad, arp, drums = [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last = bar == bars - 1
        for t, ch, ln in segments(s['chords'], bar):
            for iv in CHORD[ch]:
                pad.append((t, 48 + iv, 4 if last else ln, 66))
            if last:
                bass.append((t, bass_root(ch), 3, 96))
            elif bar >= 2:
                for k in range(int(ln * 2)):
                    bass.append((t + k * .5, bass_root(ch), .42, 92 if k % 4 == 0 else 74))
            if 6 <= bar < bars - 2:
                tones = [60 + iv for iv in CHORD[ch]] + [72 + CHORD[ch][0]]
                for k in range(int(ln * 2)):
                    arp.append((t + k * .5, tones[k % 4], .4, 62 if k % 4 == 0 else 50))
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 110), (b0, CRASH, 100)]
            continue
        if bar >= 1:
            drums += [(b0 + o, HAT, 52 if o % 1 else 64) for o in (0, .5, 1, 1.5, 2, 2.5, 3, 3.5)]
        if bar >= 2:
            drums += [(b0, KICK, 108), (b0 + 2, KICK, 100)]
        if bar >= 4:
            drums += [(b0 + 1, SNARE, 96), (b0 + 3, SNARE, 96)]
        if bar in (5, 9):
            drums += [(b0 + 3, TOM_HI, 96), (b0 + 3.33, TOM_MID, 100), (b0 + 3.66, TOM_LO, 104)]
        if bar in (2, 6, 10):
            drums.append((b0, CRASH, 92))
    ks = dict(PLAY=[(0, 1)], BASS=[(0, 2)], PAD=[(0, 1)], ARP=[(0, 1)], DRUMS=[(0, 0)])   # BELL EP, SUB, STRINGS, BELL, GATED
    return dict(BASS=bass, PAD=pad, ARP=arp, DRUMS=drums, KS=ks)


def arrange_neon_carrier(s):
    """night drive: octave-pulse bass, a supersaw pad, sixteenth arpeggios in the second half, four on the floor
    in the chorus, gated snare and clap, syndrum fills"""
    bass, pad, arp, drums = [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last, chorus, verse_b = bar == bars - 1, 18 <= bar <= 25, 10 <= bar <= 17
        for t, ch, ln in segments(s['chords'], bar):
            for iv in CHORD[ch]:
                pad.append((t, 48 + iv, 4 if last else ln, 70))
            if last:
                bass.append((t, bass_root(ch), 3, 110))
            elif bar >= 1:
                for k in range(int(ln * 2)):
                    up = chorus and k % 2 == 1
                    bass.append((t + k * .5, bass_root(ch) + (12 if up else 0), .42, 112 if k % 4 == 0 else 94))
            if verse_b or chorus:
                tones = [48 + iv for iv in CHORD[ch]] + [60 + CHORD[ch][0]]
                for k in range(int(ln * 4)):
                    arp.append((t + k * .25, tones[k % 4], .2, 72 if k % 4 == 0 else 56))
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 120), (b0, CRASH, 110)]
            continue
        if bar == 0:
            drums += [(b0 + o * .5, HAT, 60) for o in range(8)]
            continue
        if bar == 1:
            drums += [(b0 + o * .5, HAT, 70) for o in range(8)]
            drums += [(b0, KICK, 118), (b0 + 2, KICK, 118), (b0 + 3, SNARE, 80), (b0 + 3.5, SNARE, 100)]
            continue
        if chorus:
            drums += [(b0 + o, KICK, 118) for o in range(4)]
            drums += [(b0 + o + .5, OHAT, 78) for o in range(4)]
            drums += [(b0 + 1, CLAP, 92), (b0 + 3, CLAP, 92)]
        else:
            drums += [(b0, KICK, 118), (b0 + (2 if verse_b else 1.5), KICK, 98), (b0 + (2.5 if verse_b else 2), KICK, 108)]
            step = .25 if verse_b else .5
            drums += [(b0 + k * step, HAT, 72 if (k * step) % 1 == 0 else 48) for k in range(int(4 / step))]
        if bar in (9, 25):
            drums += [(b0 + 1, SNARE, 112), (b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        elif bar == 17:
            drums += [(b0 + 1, SNARE, 112)] + [(b0 + 3 + k * .25, SNARE, 90 + k * 8) for k in range(4)]
        else:
            drums += [(b0 + 1, SNARE, 114), (b0 + 3, SNARE, 114)]
        if bar in (2, 10, 18, 22):
            drums.append((b0, CRASH, 104))
    ks = dict(PLAY=[(8, 0), (40, 1), (72, 0)],          # SYN LEAD, BELL EP for the high verse, SYN LEAD again
              BASS=[(0, 0), (72, 1), (104, 0)],          # PULSE, FM BASS in the chorus, PULSE to end
              PAD=[(0, 0), (40, 1), (72, 0)],            # SUPERSAW, STRINGS in the second verse
              ARP=[(40, 0), (72, 1)],                    # PLUCK, BELL in the chorus
              LEAD=[(0, 0)], DRUMS=[(0, 0)])
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=melody(s['lead'], 84), DRUMS=drums, KS=ks)


# ---------------------------------------------------------------- two songs to hear the engine work

TOUR_PHRASES = {                                                      # four bars each, on Am F C G
    'plain': '''
0: E4 0 1 | A4 1 1 | C5 2 1 | B4 3 1
1: A4 0 2 | F4 2 1 | E4 3 1
2: G4 0 1 | E4 1 .5 | G4 1.5 .5 | C5 2 2
3: B4 0 1 | G4 1 1 | D4 2 2''',
    'eighths': '''
0: E4 0 .5 | A4 .5 .5 | C5 1 .5 | A4 1.5 .5 | E4 2 .5 | A4 2.5 .5 | B4 3 1
1: A4 0 .5 | F4 .5 .5 | A4 1 .5 | C5 1.5 .5 | F4 2 1 | E4 3 1
2: G4 0 .5 | C5 .5 .5 | E4 1 .5 | G4 1.5 .5 | C5 2 1 | B4 3 1
3: D4 0 .5 | G4 .5 .5 | B4 1 1 | G4 2 1 | D4 3 1''',
    'holds': '''
0: A4 0 2 | C5 2 2
1: A4+C5 0 4
2: G4 0 2 | E4 2 2
3: G4+B4 0 4''',
    'holds2': '''
0: A4 0 2 | C5 2 2
1: F4+A4 0 4
2: E4+G4 0 2 | G4+C5 2 2
3: D4+G4 0 2 | B4 2 2''',
}
TOUR_PLAN = [(4, 'plain', 0), (8, 'plain', 0), (12, 'eighths', 0), (16, 'eighths', 0), (20, 'holds', 0), (24, 'holds2', 0),
             (28, 'plain', 0), (32, 'plain', 12), (36, 'eighths', 0), (40, 'plain', 0), (44, 'eighths', 0)]
TOUR_LEAD = {0: [(0, 'E5', 2), (2, 'C5', 2)], 1: [(0, 'F5', 2), (2, 'A5', 2)], 2: [(0, 'G5', 4)], 3: [(0, 'D5', 2), (2, 'B4', 2)]}


def tour_play():
    out = []
    for bar, name, up in TOUR_PLAN:
        out += [(t + bar * 4, p + up, ln, v) for t, p, ln, v in melody(TOUR_PHRASES[name])]
    return out + [(192, pitch(n), 3, 100) for n in ('C4', 'E4', 'A4')]


ENGINE_TOUR = dict(
    file='engine-tour.mid', title='ENGINE TOUR', bpm=120,
    chords=['Am', 'F', 'C', 'G'] * 12 + ['Am'],
    phrases=[(6, 7), (14, 15), (22, 23), (30, 31), (38, 39), (46, 47)],
    play=tour_play())


def arrange_engine_tour(s):
    """a tour of the engine: drums alone, then bass, pad, arp, lead and keys come in one by one, each through its four
    patches two bars at a time, the clean ones first and the gritty one last (D7); the four kits and the player's five
    voices change on the way; at the end all together on the gritty ones"""
    bass, pad, arp, lead, keys, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last, finale = bar == bars - 1, 44 <= bar < bars - 1
        for t, ch, ln in segments(s['chords'], bar):
            if bar >= 4 and not last:
                bass += [(t + k * .5, bass_root(ch) + (12 if finale and k % 2 else 0), .42, 112 if k % 4 == 0 else 92) for k in range(int(ln * 2))]
            if last:
                bass.append((t, bass_root(ch), 3, 110))
            if bar >= 12:
                pad += [(t, p, 4 if last else ln, 70) for p in voicing(ch)]
            if 20 <= bar < bars - 1:
                tones = [48 + iv for iv in CHORD[ch]] + [60 + CHORD[ch][0]]
                arp += [(t + k * .25, tones[k % 4], .2, 70 if k % 4 == 0 else 54) for k in range(int(ln * 4))]
            if 28 <= bar < 36 or finale:
                down = -12 if 32 <= bar < 36 else 0                    # under the play track when it goes up an octave
                lead += [(t + o, pitch(n) + down, d, 80) for o, n, d in TOUR_LEAD[bar % 4]]
            if 36 <= bar < 44 or finale:
                keys += [(t + o, p, .3, 76 if o == .5 else 64) for o in (.5, 1.5, 2.5, 3.5) for p in voicing(ch, 60)]
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 120), (b0, CRASH, 110)]
            continue
        drums += [(b0, KICK, 118), (b0 + 2, KICK, 112)]
        drums += [(b0 + k * .5, HAT, 72 if k % 2 == 0 else 50) for k in range(8)]
        if bar >= 1:
            drums += [(b0 + 1, SNARE, 112), (b0 + 3, SNARE, 112)]
        if bar >= 2:
            drums.append((b0 + 2.5, KICK, 96))
        if (bar >= 2 and bar % 4 == 2) or finale:
            drums += [(b0 + 1, CLAP, 90), (b0 + 3, CLAP, 90)]
        if finale:
            drums += [(b0 + o + .5, OHAT, 70) for o in range(4)] + [(b0 + 1, KICK, 104), (b0 + 3, KICK, 104)]
        if bar % 8 == 7 or bar == 3:
            drums += [(b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        if bar % 4 == 0 and bar >= 4:
            drums.append((b0, CRASH, 100))
    end = 176                                                          # the finale, bar 45: every part on its gritty patch
    ks = dict(PLAY=[(16, 0), (48, 1), (80, 2), (112, 3), (144, 4)],    # SYN LEAD, BELL EP, BRASS, GLASS, FUZZ
              BASS=[(16, 0), (24, 1), (32, 2), (40, 3), (48, 0), (end, 3)],
              PAD=[(48, 0), (56, 1), (64, 2), (72, 3), (80, 0), (end, 3)],
              ARP=[(80, 0), (88, 1), (96, 2), (104, 3), (112, 0), (end, 3)],
              LEAD=[(112, 0), (120, 1), (128, 2), (136, 3), (end, 3)],
              KEYS=[(144, 0), (152, 1), (160, 2), (168, 3), (end, 3)],
              DRUMS=[(0, 0), (48, 1), (96, 2), (144, 3)])               # GATED, TIGHT, BOOM, DIRT
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=lead, KEYS=keys, DRUMS=drums, KS=ks)


CHROME_HORIZON = dict(
    file='chrome-horizon.mid', title='CHROME HORIZON', bpm=104,
    chords=['Dm', 'Bb', 'F', 'C'] * 3 + ['Gm', 'Bb', 'C', 'C'] + ['Dm', 'Bb', 'F', 'C'] * 2 + ['Bb', 'F', 'C', 'Dm']
           + ['Dm', 'Bb', 'F', 'C'] * 2 + ['Dm'],
    phrases=[(6, 7), (13, 14), (18, 19), (22, 23), (28, 29), (30, 31)],
    solos=[(24, 27)],                 # D14: a FREE SOLO over the breakdown, bars 25-28: the player's own
    play='''
4: A4 0 1 | D5 1 1 | C5 2 .5 | A4 2.5 .5 | F4 3 1
5: A#4 0 1.5 | A4 1.5 .5 | G4 2 2
6: A4 0 1 | C5 1 1 | F5 2 1 | E5 3 1
7: D5 0 .5 | C5 .5 .5 | G4 1 1 | C5 2 2
8: A4 0 1 | D5 1 1 | E5 2 .5 | D5 2.5 .5 | A4 3 1
9: A#4 0 1 | D5 1 1 | F5 2 2
10: E5 0 1 | D5 1 .5 | C5 1.5 .5 | A4 2 1 | C5 3 1
11: G4 0 3
12: G4 0 2 | A#4 2 2
13: D5 0 2 | F5 2 2
14: E5 0 2 | C5 2 2
15: G4 0 1 | A4 1 1 | A#4 2 1 | C5 3 1
16: D5 0 1.5 | A4 1.5 .5 | D5 2 1 | E5 3 1
17: F5 0 1.5 | E5 1.5 .5 | D5 2 2
18: C5 0 1 | A4 1 .5 | C5 1.5 .5 | F5 2 1 | E5 3 1
19: D5 0 .5 | C5 .5 .5 | G4 1 1 | C5 2 2
20: D5 0 1.5 | A4 1.5 .5 | D5 2 1 | F5 3 1
21: D5+F5 0 2 | A#4 2 1 | D5 3 1
22: C5 0 1 | A4 1 1 | C5 2 1 | E5 3 1
23: C5+E5 0 3
28: D5 0 1.5 | A4 1.5 .5 | D5 2 1 | E5 3 1
29: F5 0 1.5 | E5 1.5 .5 | D5 2 2
30: C5 0 1 | A4 1 .5 | C5 1.5 .5 | F5 2 1 | E5 3 1
31: D5 0 .5 | C5 .5 .5 | G4 1 1 | C5 2 2
32: D6 0 1.5 | A5 1.5 .5 | D6 2 1 | F6 3 1
33: F6 0 1.5 | E6 1.5 .5 | D6 2 2
34: C6 0 1 | A5 1 .5 | C6 1.5 .5 | F6 2 1 | E6 3 1
35: D6 0 1 | C6 1 1 | A#5 2 1 | A5 3 1
36: F4+A4+D5 0 3''',
    lead='''
16: F4 0 2 | A4 2 2
17: D4 0 4
18: A4 0 2 | C5 2 2
19: G4 0 2 | E4 2 2
20: F4 0 2 | A4 2 2
21: A#4 0 4
22: C5 0 2 | A4 2 2
23: G4 0 4
28: F4 0 2 | A4 2 2
29: D4 0 4
30: A4 0 2 | C5 2 2
31: G4 0 2 | E4 2 2
32: F5 0 2 | A5 2 2
33: D5 0 4
34: A5 0 2 | C6 2 2
35: G5 0 2 | E5 2 2''')


def arrange_chrome_horizon(s):
    """a synthwave song in D minor, gritty in its choruses (D7), its play track on white keys only after the import's transpose:
    intro on a glass pad and a bell arp, verse with pulse bass and the tight kit, a brass pre-chorus, a chorus on the
    gated kit with FM bass, supersaw and a saw lead, a breakdown on the boom kit with tine keys, the last chorus up an octave"""
    bass, pad, arp, keys, stabs, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last = bar == bars - 1
        intro, verse, pre = bar < 4, 4 <= bar < 12, 12 <= bar < 16
        chorus, brk, final = 16 <= bar < 24, 24 <= bar < 28, 28 <= bar < bars - 1
        for t, ch, ln in segments(s['chords'], bar):
            for p in voicing(ch, 50):
                pad.append((t, p, 4 if last else ln, 64 if intro or brk else 72))
            r = bass_root(ch)
            if last:
                bass.append((t, r, 3, 112))
            elif verse:
                bass += [(t + k * .5, r, .42, 108 if k % 4 == 0 else 88) for k in range(8)]
            elif pre or brk:
                bass += [(t, r, 1.9, 104), (t + 2, r, 1.9, 96)]
            elif chorus or final:
                bass += [(t + k * .5, r + (12 if k % 2 else 0), .42, 114 if k % 4 == 0 else 96) for k in range(8)]
            tones = [60 + iv for iv in CHORD[ch]] + [72 + CHORD[ch][0]]
            if intro or verse:
                arp += [(t + k * .5, tones[k % 4], .45, 60 if k % 4 == 0 else 48) for k in range(8)]
            elif chorus or final:
                lo = [48 + iv for iv in CHORD[ch]] + [60 + CHORD[ch][0]]
                arp += [(t + k * .25, lo[k % 4], .2, 68 if k % 4 == 0 else 52) for k in range(16)]
            if pre:
                stabs += [(t + o, p, .5, 96) for o in (0, 2) for p in voicing(ch, 62)]
            if brk:
                keys += [(t + k, tones[k % 4], .9, 70) for k in range(4)]
            if final:
                keys += [(t + o, p, .3, 60) for o in (.5, 2.5) for p in voicing(ch, 62)]
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 120), (b0, CRASH, 110)]
            continue
        if intro:
            if bar == 3:
                drums += [(b0 + 2 + k * .5, SNARE, 70 + k * 10) for k in range(4)]
            continue
        if verse:
            drums += [(b0, KICK, 116), (b0 + 2, KICK, 110), (b0 + 1, SNARE, 108), (b0 + 3, SNARE, 108)]
            drums += [(b0 + k * .25, HAT, 70 if k % 2 == 0 else 44) for k in range(16)]
        elif pre:
            drums += [(b0, KICK, 116), (b0 + 2, KICK, 110), (b0 + 1, SNARE, 110), (b0 + 3, SNARE, 110)]
            drums += [(b0 + k * .5, HAT, 66) for k in range(8)]
            if bar == 15:
                drums += [(b0 + 2 + k * .25, SNARE, 80 + k * 6) for k in range(8)]
        elif brk:
            drums += [(b0, KICK, 118)]
            if bar % 2 == 1:
                drums += [(b0 + 2, TOM_HI, 96), (b0 + 2.5, TOM_MID, 100), (b0 + 3, TOM_LO, 104), (b0 + 3.5, TOM_LO, 108)]
        else:                                                          # the choruses: four on the floor
            drums += [(b0 + o, KICK, 118) for o in range(4)]
            drums += [(b0 + 1, SNARE, 112), (b0 + 3, SNARE, 112), (b0 + 1, CLAP, 90), (b0 + 3, CLAP, 90)]
            drums += [(b0 + o + .5, OHAT, 76) for o in range(4)]
            if bar in (23, 35):
                drums += [(b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        if bar in (4, 12, 16, 24, 28, 32):
            drums.append((b0, CRASH, 102))
    lead = melody(s['lead'], 78)
    ks = dict(PLAY=[(16, 1), (48, 2), (64, 0), (96, 3), (112, 4)],      # BELL EP, BRASS, SYN LEAD, GLASS, FUZZ
              BASS=[(16, 0), (48, 2), (64, 3), (96, 2), (112, 3)],      # PULSE, SUB, DIRT BASS, SUB, DIRT BASS
              PAD=[(0, 2), (16, 1), (48, 0), (64, 3), (96, 2), (112, 3)],  # GLASS PAD, STRINGS, SUPERSAW, GRIT PAD, GLASS PAD, GRIT PAD
              ARP=[(0, 1), (64, 0), (112, 3)],                          # BELL, PLUCK, CRUSH
              LEAD=[(48, 2), (64, 3)],                                  # BRASS LEAD stabs, FUZZ LEAD
              KEYS=[(96, 1), (112, 3)],                                 # TINE, GRIT EP
              DRUMS=[(16, 1), (48, 0), (64, 3), (96, 2), (112, 3)])     # TIGHT, GATED, DIRT, BOOM, DIRT
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=stabs + lead, KEYS=keys, DRUMS=drums, KS=ks)


# KIRAMEKI RUSH: arcade J-pop, fast and hard (EXP). The royal road (IV V iii vi) the genre leans on, a hook in the
# intro, a pre-chorus that climbs to the III chord (E major), a busy chorus, a run, the chorus again an octave up.
KIRAMEKI_TEXT = """
0: A5 0 .5 | C6 .5 .5 | G5 1 1 | F5 2 .5 | G5 2.5 .5 | A5 3 1
1: B5 0 .5 | G5 .5 .5 | D5 1 1 | G5 2 .5 | A5 2.5 .5 | B5 3 1
2: C6 0 .5 | B5 .5 .5 | G5 1 .5 | E5 1.5 .5 | G5 2 .5 | B5 2.5 .5 | E5 3 1
3: A5 0 1.5 | G5 1.5 .5 | E5 2 .5 | C5 2.5 1.5
4: E4 0 .5 | E4 .5 .5 | G4 1 .5 | C5 1.5 1 | B4 2.5 .5 | G4 3 1
5: D4 0 .5 | D4 .5 .5 | G4 1 .5 | B4 1.5 1 | A4 2.5 .5 | G4 3 .5 | D4 3.5 .5
6: E4 0 .5 | C4 .5 .5 | E4 1 .5 | A4 1.5 1 | G4 2.5 .5 | E4 3 1
7: G4 0 1.5 | E4 1.5 .5 | D4 2 .5 | E4 2.5 .5 | G4 3 .5 | B4 3.5 .5
8: C5 0 .5 | A4 .5 .5 | F4 1 .5 | A4 1.5 1 | G4 2.5 .5 | F4 3 1
9: E4 0 .5 | G4 .5 .5 | C5 1 1.5 | B4 2.5 .5 | A4 3 .5 | G4 3.5 .5
10: F4 0 .5 | E4 .5 .5 | D4 1 .5 | F4 1.5 .5 | A4 2 1 | C5 3 1
11: B4 0 2 | A4 2 .5 | G4 2.5 .5 | D4 3 1
12: A4 0 1 | A4 1 .5 | G4 1.5 .5 | A4 2 1 | C5 3 1
13: B4 0 1 | B4 1 .5 | A4 1.5 .5 | B4 2 1 | G4 3 1
14: G4 0 .5 | A4 .5 .5 | B4 1 .5 | C5 1.5 .5 | B4 2 .5 | A4 2.5 .5 | G4 3 .5 | E4 3.5 .5
15: B4 0 1.5 | A4 1.5 .5 | B4 2 .5 | E4 2.5 .5 | B4 3 1
16: A5 0 .5 | G5 .5 .5 | A5 1 .5 | C6 1.5 1 | A5 2.5 .5 | G5 3 .5 | F5 3.5 .5
17: G5 0 .5 | D5 .5 .5 | G5 1 .5 | B5 1.5 1 | A5 2.5 .5 | G5 3 .5 | D5 3.5 .5
18: E5 0 .5 | G5 .5 .5 | B5 1 .5 | C6 1.5 .5 | B5 2 .5 | G5 2.5 .5 | E5 3 .5 | G5 3.5 .5
19: A5+C6 0 1.5 | G5 1.5 .5 | E5 2 .5 | G5 2.5 .5 | A5 3 1
20: A5 0 .5 | G5 .5 .5 | A5 1 .5 | C6 1.5 1 | A5 2.5 .5 | C6 3 .5 | A5 3.5 .5
21: B5 0 .5 | A5 .5 .5 | G5 1 .5 | D5 1.5 .5 | G5 2 .5 | A5 2.5 .5 | B5 3 .5 | C6 3.5 .5
22: E5+G5+C6 0 2 | G5 2 .5 | A5 2.5 .5 | B5 3 .5 | C6 3.5 .5
23: C6 0 2 | G5 2 .5 | E5 2.5 .5 | C5 3 1
24: D5 0 .5 | F5 .5 .5 | A5 1 .5 | D5 1.5 .5 | F5 2 .5 | A5 2.5 .5 | C6 3 .5 | A5 3.5 .5
25: E5 0 .5 | G5 .5 .5 | B5 1 .5 | E5 1.5 .5 | G5 2 .5 | B5 2.5 .5 | C6 3 .5 | B5 3.5 .5
26: F5 0 .5 | A5 .5 .5 | C6 1 .5 | A5 1.5 .5 | F5+A5 2 1 | G5 3 .5 | A5 3.5 .5
27: G5+B5 0 1 | D5 1 .5 | G5 1.5 .5 | B5 2 .5 | A5 2.5 .5 | G5 3 1
36: C6+E6+G6 0 3"""


def kirameki_play():
    """the play track: as written, and its chorus (bars 17-24) again an octave up in bars 29-36"""
    notes = melody(KIRAMEKI_TEXT)
    return notes + [(t + 48, p + 12, ln, v) for t, p, ln, v in notes if 64 <= t < 96]


KIRAMEKI_RUSH = dict(
    file='kirameki-rush.mid', title='KIRAMEKI RUSH', bpm=174,
    chords=['F', 'G', 'Em', 'Am'] + ['C', 'G', 'Am', 'Em', 'F', 'C', 'Dm', 'G'] + ['F', 'G', 'Em', 'E']
           + ['F', 'G', 'Em', 'Am', 'F', 'G', 'C', 'C'] + ['Dm', 'Em', 'F', 'G'] + ['F', 'G', 'Em', 'Am', 'F', 'G', 'C', 'C']
           + ['C'],
    phrases=[(2, 3), (8, 9), (14, 15), (18, 19), (22, 23), (26, 27), (30, 31), (34, 35)],
    play=kirameki_play())


def arrange_kirameki_rush(s):
    """arcade J-pop at 174 BPM in C major: a glass pad and a pluck arp under the intro's hook, a verse on bell EP with EP
    stabs and pulse bass, a pre-chorus over four on the floor that climbs to E major, choruses with octave bass, a
    supersaw, a saw lead's long notes and sixteenth arps, a run over rising chords, the last chorus an octave up"""
    bass, pad, arp, keys, lead, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last = bar == bars - 1
        intro, verse, pre = bar < 4, 4 <= bar < 12, 12 <= bar < 16
        chorus, run, final = 16 <= bar < 24, 24 <= bar < 28, 28 <= bar < bars - 1
        big = chorus or final
        for t, ch, ln in segments(s['chords'], bar):
            r = bass_root(ch)
            for p in voicing(ch, 55 if big or run else 52):
                pad.append((t, p, 4 if last else ln, 70 if big else 58))
            if last:
                bass.append((t, r, 3, 112))
            elif intro and bar < 2:
                bass.append((t, r, 3.8, 96))
            elif verse:
                bass += [(t + k * .5, r + (12 if k % 2 else 0), .4, 102 if k % 4 == 0 else 84) for k in range(8)]
            else:                                                       # the octave bass of the genre
                bass += [(t + k * .5, r + (12 if k % 2 else 0), .4, 112 if k % 2 == 0 else 94) for k in range(8)]
            tones = sorted(72 + iv % 12 for iv in CHORD[ch]) + [84 + CHORD[ch][0] % 12]
            if verse:
                arp += [(t + k * .5, tones[k % 4] - 12, .4, 54 if k % 4 == 0 else 44) for k in range(8)]
                keys += [(t + o, p, .3, 64) for o in (.5, 1.5, 2.5, 3.5) for p in voicing(ch, 60)]
            elif not last and not (intro and bar < 2):
                arp += [(t + k * .25, tones[k % 4], .2, 62 if k % 4 == 0 else 48) for k in range(16)]
            if big:
                top = voicing(ch, 64)
                lead += [(t, top[2], 1.9, 64), (t + 2, top[1], 1.9, 60)]
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 120), (b0, CRASH, 112)]
            continue
        if intro:
            if bar >= 2:
                drums += [(b0 + o, KICK, 112) for o in range(4)] + [(b0 + o + .5, OHAT, 70) for o in range(4)]
            if bar == 3:
                drums += [(b0 + 2 + k * .25, SNARE, 70 + k * 6) for k in range(8)]
            continue
        if verse:
            drums += [(b0, KICK, 116), (b0 + 1.5, KICK, 98), (b0 + 2, KICK, 112), (b0 + 1, SNARE, 106), (b0 + 3, SNARE, 106)]
            drums += [(b0 + k * .25, HAT, 66 if k % 2 == 0 else 40) for k in range(16)]
        elif pre:
            drums += [(b0 + o, KICK, 116) for o in range(4)] + [(b0 + 1, SNARE, 108), (b0 + 3, SNARE, 108)]
            drums += [(b0 + k * .5, HAT, 64) for k in range(8)]
            if bar == 15:
                drums += [(b0 + 2 + k * .25, SNARE, 84 + k * 5) for k in range(8)]
        else:                                                           # the choruses and the run: four on the floor
            drums += [(b0 + o, KICK, 118) for o in range(4)]
            drums += [(b0 + 1, SNARE, 112), (b0 + 3, SNARE, 112), (b0 + 1, CLAP, 92), (b0 + 3, CLAP, 92)]
            drums += [(b0 + o + .5, OHAT, 78) for o in range(4)]
            if run:
                drums += [(b0 + k * .25, HAT, 48) for k in range(16) if k % 2]
            if bar in (23, 27, 35):
                drums += [(b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        if bar in (4, 12, 16, 24, 28, 32):
            drums.append((b0, CRASH, 104))
    ks = dict(PLAY=[(0, 0), (16, 1), (48, 3), (64, 0), (96, 2), (112, 0)],   # SYN LEAD, BELL EP, GLASS, SYN LEAD, BRASS, SYN LEAD
              BASS=[(0, 1), (16, 0), (48, 1)],                              # FM BASS, PULSE, FM BASS
              PAD=[(0, 2), (16, 1), (64, 0)],                               # GLASS PAD, STRINGS, SUPERSAW
              ARP=[(0, 0), (16, 1), (48, 0)],                               # PLUCK, BELL, PLUCK
              KEYS=[(16, 0)],                                               # FM EP
              LEAD=[(64, 0)],                                               # SAW LEAD
              DRUMS=[(0, 0), (16, 1), (48, 0)])                             # GATED, TIGHT, GATED
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=lead, KEYS=keys, DRUMS=drums, KS=ks)


# YOZORA SPARK: arcade J-pop as KIRAMEKI RUSH, faster and harder (EXP), on the genre's best-loved progressions: the
# Komuro progression (vi IV V I) in the intro and the verse, "Just the Two of Us" (IV III7 vi, v I7) in the
# pre-chorus, the royal road (IV V iii vi) in the chorus, the last chorus a whole step up (C to D major: its melody
# keeps to D E G A B, so it stays on the white keys; the band has the F# and C#), and the arcade's cadence to end on,
# bVI bVII I (Bb C D). Sixteenth pairs on one key and runs make it harder.
YOZORA_TEXT = """
0: E5 0 .5 | A5 .5 .5 | B5 1 .5 | C6 1.5 .5 | B5 2 .5 | A5 2.5 .5 | E5 3 .5 | G5 3.5 .5
1: A5 0 .5 | F5 .5 .5 | A5 1 .5 | C6 1.5 1 | A5 2.5 .5 | G5 3 .5 | F5 3.5 .5
2: G5 0 .5 | B5 .5 .5 | D5 1 .5 | G5 1.5 .5 | B5 2 .25 | A5 2.25 .25 | G5 2.5 .5 | D5 3 1
3: E5+G5 0 1 | C6 1 .5 | B5 1.5 .5 | G5 2 .5 | E5 2.5 .5 | C5 3 1
4: A4 0 .5 | A4 .5 .25 | A4 .75 .25 | C5 1 .5 | B4 1.5 .5 | A4 2 .5 | E4 2.5 .5 | G4 3 .5 | A4 3.5 .5
5: F4 0 .5 | A4 .5 .5 | C5 1 1 | B4 2 .5 | A4 2.5 .5 | F4 3 1
6: G4 0 .5 | G4 .5 .25 | G4 .75 .25 | B4 1 .5 | A4 1.5 .5 | G4 2 .5 | D4 2.5 .5 | G4 3 1
7: E4 0 .5 | G4 .5 .5 | C5 1 1.5 | B4 2.5 .5 | C5 3 .5 | E4 3.5 .5
8: A4 0 .5 | C5 .5 .5 | E4 1 .5 | A4 1.5 .5 | C5 2 .5 | B4 2.5 .25 | A4 2.75 .25 | G4 3 1
9: F4 0 .5 | A4 .5 .5 | C5 1 1 | A4 2 .5 | F4 2.5 .5 | E4 3 1
10: G4 0 1 | B4 1 .5 | A4 1.5 .5 | G4 2 .5 | F4 2.5 .5 | E4 3 .5 | D4 3.5 .5
11: E4 0 .5 | B4 .5 .5 | E4 1 .5 | B4 1.5 .5 | D4 2 .5 | E4 2.5 .5 | B4 3 1
12: A4 0 .5 | C5 .5 .5 | E4 1 .5 | F4 1.5 .5 | A4 2 1 | C5 3 .5 | A4 3.5 .5
13: B4 0 1 | E4 1 .5 | B4 1.5 .5 | A4 2 .5 | B4 2.5 .5 | E4 3 1
14: C5 0 .5 | B4 .5 .5 | A4 1 .5 | E4 1.5 .5 | A4 2 .5 | B4 2.5 .5 | C5 3 1
15: G4 0 .5 | D4 .5 .5 | G4 1 1 | C5 2 .5 | E4 2.5 .5 | G4 3 1
16: A5+C6 0 .5 | A5 .5 .5 | G5 1 .5 | A5 1.5 .5 | C6 2 .5 | A5 2.5 .5 | G5 3 .5 | F5 3.5 .5
17: G5+B5 0 .5 | G5 .5 .5 | D5 1 .5 | G5 1.5 .5 | B5 2 .5 | A5 2.5 .25 | G5 2.75 .25 | D5 3 1
18: E5+G5 0 .5 | B5 .5 .5 | E5 1 .5 | G5 1.5 .5 | B5 2 .5 | C6 2.5 .5 | B5 3 .5 | G5 3.5 .5
19: A5+C6 0 1.5 | G5 1.5 .5 | A5 2 .5 | C6 2.5 .5 | E5 3 1
20: F5+A5 0 .5 | D5 .5 .5 | F5 1 .5 | A5 1.5 .5 | C6 2 .5 | A5 2.5 .5 | F5 3 .5 | D5 3.5 .5
21: G5+B5 0 .5 | D5 .5 .5 | G5 1 .5 | B5 1.5 .5 | A5 2 .25 | B5 2.25 .25 | C6 2.5 .5 | B5 3 .5 | G5 3.5 .5
22: E5+G5+C6 0 2 | G5 2 .5 | A5 2.5 .5 | B5 3 .5 | C6 3.5 .5
23: C6 0 1 | B5 1 .5 | G5 1.5 .5 | E5 2 .5 | D5 2.5 .5 | C5 3 1
24: D5 0 .25 | F5 .25 .25 | A5 .5 .5 | D5 1 .5 | A5 1.5 .5 | C6 2 .5 | A5 2.5 .5 | F5 3 1
25: E5 0 .25 | G5 .25 .25 | B5 .5 .5 | E5 1 .5 | B5 1.5 .5 | C6 2 .5 | B5 2.5 .5 | G5 3 1
26: F5+A5 0 .5 | C6 .5 .5 | A5 1 .5 | F5 1.5 .5 | A5 2 .5 | C6 2.5 .5 | A5 3 1
27: G5+B5 0 .5 | D5 .5 .5 | G5 1 .5 | B5 1.5 .5 | E5+A5 2 1
28: G6+B6 0 .5 | G6 .5 .5 | E6 1 .5 | G6 1.5 .5 | B6 2 .5 | G6 2.5 .5 | E6 3 .5 | D6 3.5 .5
29: E6+A6 0 .5 | E6 .5 .5 | A6 1 .5 | B6 1.5 .5 | A6 2 .25 | B6 2.25 .25 | A6 2.5 .5 | E6 3 1
30: A6 0 .5 | E6 .5 .5 | A6 1 .5 | B6 1.5 .5 | A6 2 1 | E6 3 1
31: D6+B6 0 1.5 | A6 1.5 .5 | B6 2 .5 | D6 2.5 .5 | E6 3 1
32: E6+G6 0 .5 | B6 .5 .5 | G6 1 .5 | E6 1.5 .5 | G6 2 .5 | B6 2.5 .5 | A6 3 .5 | G6 3.5 .5
33: E6+A6 0 .5 | B6 .5 .5 | A6 1 .5 | E6 1.5 .5 | A6 2 .25 | B6 2.25 .25 | A6 2.5 .5 | E6 3 .5 | G6 3.5 .5
34: D6+A6 0 2 | B6 2 .5 | A6 2.5 .5 | G6 3 .5 | E6 3.5 .5
35: D6 0 1 | E6 1 .5 | G6 1.5 .5 | A6 2 1 | B6 3 1
36: D6+F6 0 1 | F6 1 .5 | D6 1.5 .5 | E6+G6 2 1 | G6 3 .5 | A6 3.5 .5
37: D6+A6 0 3"""

YOZORA_SPARK = dict(
    file='yozora-spark.mid', title='YOZORA SPARK', bpm=180,
    chords=['Am', 'F', 'G', 'C'] + ['Am', 'F', 'G', 'C', 'Am', 'F', 'G', 'E'] + ['F', 'E', 'Am', 'Gm C']
           + ['F', 'G', 'Em', 'Am', 'Dm', 'G', 'C', 'C'] + ['Dm', 'Em', 'F', 'G A']
           + ['G', 'A', 'F#m', 'Bm', 'Em', 'A', 'D', 'D'] + ['Bb C', 'D'],
    phrases=[(2, 3), (8, 9), (14, 15), (18, 19), (22, 23), (26, 27), (30, 31), (34, 35)],
    play=YOZORA_TEXT)


def arrange_yozora_spark(s):
    """arcade J-pop at 180 BPM, as KIRAMEKI RUSH: a pluck arp and a glass pad under the intro's hook, a verse on bell EP
    with EP stabs and octave bass, a pre-chorus with a syncopated bass under "Just the Two of Us", choruses on four on
    the floor with a supersaw, a saw lead's long notes and sixteenth arps, a run, the last chorus a whole step up, the
    cadence bVI bVII I and a held D"""
    bass, pad, arp, keys, lead, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        end = bar >= bars - 2
        intro, verse, pre = bar < 4, 4 <= bar < 12, 12 <= bar < 16
        chorus, run, final = 16 <= bar < 24, 24 <= bar < 28, 28 <= bar < bars - 2
        big = chorus or final
        for t, ch, ln in segments(s['chords'], bar):
            r, n8, n16 = bass_root(ch), int(round(ln * 2)), int(round(ln * 4))
            for p in voicing(ch, 55 if big or run or end else 52):
                pad.append((t, p, ln if bar < bars - 1 else 4, 72 if big or end else 58))
            if bar == bars - 1:
                bass.append((t, r, 3, 114))
            elif intro and bar < 2:
                bass.append((t, r, ln - .2, 96))
            elif pre:                                                   # the syncopation of "Just the Two of Us"
                bass += [(t + o, r + (12 if o in (.75, 2.75) else 0), .35, 104 if o in (0, 2) else 90)
                         for o in (0, .75, 1.5, 2, 2.75, 3.5) if o < ln]
            else:                                                       # the octave bass
                bass += [(t + k * .5, r + (12 if k % 2 else 0), .4, 112 if k % 2 == 0 else 94) for k in range(n8)]
            tones = sorted(72 + iv % 12 for iv in CHORD[ch]) + [84 + CHORD[ch][0] % 12]
            if verse or pre:
                arp += [(t + k * .5, tones[k % 4] - 12, .4, 54 if k % 4 == 0 else 44) for k in range(n8)]
                keys += [(t + k + .5, p, .3, 64) for k in range(int(ln)) for p in voicing(ch, 60)]
            elif bar < bars - 1 and not (intro and bar < 2):
                arp += [(t + k * .25, tones[k % 4], .2, 62 if k % 4 == 0 else 48) for k in range(n16)]
            if big or end:
                top = voicing(ch, 64)
                lead += [(t, top[2], min(1.9, ln - .1), 64)] + ([(t + 2, top[1], 1.9, 60)] if ln >= 4 else [])
        b0 = bar * 4
        if bar == bars - 1:
            drums += [(b0, KICK, 122), (b0, CRASH, 114)]
            continue
        if intro:
            if bar >= 2:
                drums += [(b0 + o, KICK, 112) for o in range(4)] + [(b0 + o + .5, OHAT, 70) for o in range(4)]
            if bar == 3:
                drums += [(b0 + 2 + k * .25, SNARE, 70 + k * 6) for k in range(8)]
            continue
        if verse:
            drums += [(b0, KICK, 116), (b0 + 1.5, KICK, 98), (b0 + 2, KICK, 112), (b0 + 1, SNARE, 106), (b0 + 3, SNARE, 106)]
            drums += [(b0 + k * .25, HAT, 66 if k % 2 == 0 else 40) for k in range(16)]
        elif pre:
            drums += [(b0, KICK, 116), (b0 + .75, KICK, 96), (b0 + 2, KICK, 112), (b0 + 2.75, KICK, 96),
                      (b0 + 1, SNARE, 108), (b0 + 3, SNARE, 108)] + [(b0 + k * .5, HAT, 64) for k in range(8)]
            if bar == 15:
                drums += [(b0 + 2 + k * .25, SNARE, 84 + k * 5) for k in range(8)]
        else:                                                           # the choruses, the run, the cadence
            drums += [(b0 + o, KICK, 118) for o in range(4)]
            drums += [(b0 + 1, SNARE, 112), (b0 + 3, SNARE, 112), (b0 + 1, CLAP, 92), (b0 + 3, CLAP, 92)]
            drums += [(b0 + o + .5, OHAT, 78) for o in range(4)]
            if run:
                drums += [(b0 + k * .25, HAT, 48) for k in range(16) if k % 2]
            if bar in (23, 27, 35):
                drums += [(b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        if bar in (4, 12, 16, 24, 28, 32, 36):
            drums.append((b0, CRASH, 104))
    ks = dict(PLAY=[(0, 0), (16, 1), (48, 3), (64, 0), (96, 2), (112, 0)],   # SYN LEAD, BELL EP, GLASS, SYN LEAD, BRASS, SYN LEAD
              BASS=[(0, 1)],                                                # FM BASS
              PAD=[(0, 2), (16, 1), (64, 0)],                               # GLASS PAD, STRINGS, SUPERSAW
              ARP=[(0, 0), (16, 1), (64, 0)],                               # PLUCK, BELL, PLUCK
              KEYS=[(16, 0)],                                               # FM EP
              LEAD=[(64, 0)],                                               # SAW LEAD
              DRUMS=[(0, 0), (16, 1), (48, 0)])                             # GATED, TIGHT, GATED
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=lead, KEYS=keys, DRUMS=drums, KS=ks)


# TURBO NOIR: outrun after Kavinsky's night drives (an original song). A minor at 112 BPM: a robotic, staccato line over a
# pulsing sixteenth bass and a square sequence, a pre-chorus that hangs on E major, a chorus of long synth-brass notes
# over saw chords and a gated snare, a breakdown, the last chorus an octave up on the dirty bass.
TURBO_TEXT = """
0: A4 0 .5 | A4 1 .5 | C5 1.5 .5 | B4 2 1 | E4 3 1
1: A4 0 .5 | A4 1 .5 | C5 1.5 .5 | B4 2 .5 | A4 2.5 .5 | G4 3 1
2: F4 0 .5 | F4 1 .5 | A4 1.5 .5 | G4 2 1 | C4 3 1
3: F4 0 .5 | F4 1 .5 | A4 1.5 .5 | C5 2 1.5 | B4 3.5 .5
4: E4 0 .5 | A4 .5 .5 | E4 1 .5 | C5 1.5 .5 | B4 2 .5 | A4 2.5 .5 | E4 3 1
5: E4 0 .5 | A4 .5 .5 | E4 1 .5 | C5 1.5 .5 | B4 2 1 | G4 3 1
6: F4 0 .5 | A4 .5 .5 | F4 1 .5 | C5 1.5 .5 | B4 2 .5 | A4 2.5 .5 | F4 3 1
7: F4 0 .5 | A4 .5 .5 | F4 1 .5 | C5 1.5 .5 | A4 2 1.5 | E4 3.5 .5
8: G4 0 .5 | C5 .5 .5 | G4 1 .5 | E4 1.5 .5 | G4 2 .5 | A4 2.5 .5 | G4 3 1
9: E4 0 .5 | G4 .5 .5 | C5 1 1 | B4 2 .5 | C5 2.5 .5 | G4 3 1
10: D4 0 .5 | G4 .5 .5 | B4 1 .5 | G4 1.5 .5 | B4 2 .5 | A4 2.5 .5 | G4 3 1
11: B4 0 2 | A4 2 .5 | G4 2.5 .5 | D4 3 1
12: D4+F4 0 1 | A4 1 1 | F4 2 .5 | A4 2.5 .5 | C5 3 1
13: D4+A4 0 1.5 | C5 1.5 .5 | A4 2 1 | F4 3 1
14: E4+B4 0 1.5 | E4 1.5 .5 | B4 2 1 | A4 3 .5 | B4 3.5 .5
15: B4 0 2 | C5 2 .5 | B4 2.5 .5 | E4 3 1
16: A5 0 1.5 | E5 1.5 .5 | A5 2 .5 | C6 2.5 1.5
17: A5 0 1.5 | F5 1.5 .5 | A5 2 .5 | C6 2.5 .5 | B5 3 .5 | A5 3.5 .5
18: G5 0 1.5 | E5 1.5 .5 | G5 2 .5 | C6 2.5 1.5
19: B5 0 1 | A5 1 .5 | G5 1.5 .5 | D5 2 2
20: E5+A5 0 1.5 | C6 1.5 .5 | B5 2 .5 | A5 2.5 .5 | E5 3 1
21: F5+A5 0 1.5 | C6 1.5 .5 | A5 2 .5 | F5 2.5 .5 | A5 3 1
22: E5+B5 0 2 | C6 2 .5 | B5 2.5 .5 | A5 3 .5 | B5 3.5 .5
23: E5+B5 0 3
24: C6 0 2 | B5 2 1 | A5 3 1
25: E5 0 3
26: A5 0 2 | G5 2 1 | F5 3 1
27: F5+A5 0 2
36: E6+A6 0 3"""


def turbo_play():
    """the play track: as written, and its chorus (bars 17-24) again an octave up in bars 29-36"""
    notes = melody(TURBO_TEXT)
    return notes + [(t + 48, p + 12, ln, v) for t, p, ln, v in notes if 64 <= t < 96]


TURBO_NOIR = dict(
    file='turbo-noir.mid', title='TURBO NOIR', bpm=112,
    chords=['Am', 'Am', 'F', 'F'] + ['Am', 'Am', 'F', 'F', 'C', 'C', 'G', 'G'] + ['Dm', 'Dm', 'E', 'E']
           + ['Am', 'F', 'C', 'G', 'Am', 'F', 'E', 'E'] + ['Am', 'Am', 'F', 'F']
           + ['Am', 'F', 'C', 'G', 'Am', 'F', 'E', 'E'] + ['Am'],
    phrases=[(2, 3), (8, 9), (14, 15), (18, 19), (22, 23), (30, 31), (34, 35)],
    play=turbo_play())


def arrange_turbo_noir(s):
    """outrun at 112 BPM in A minor: a glass pad and a bell arp in the intro, a verse on a pulsing sixteenth bass and a
    square sequence over strings, brass stabs on the pre-chorus's E major, choruses on four on the floor with a gated
    snare, saw chords and octave bass, a breakdown on the pad and a kick, the last chorus on the dirty bass"""
    bass, pad, arp, lead, keys, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    for bar in range(bars):
        last = bar == bars - 1
        intro, verse, pre = bar < 4, 4 <= bar < 12, 12 <= bar < 16
        chorus, brk, final = 16 <= bar < 24, 24 <= bar < 28, 28 <= bar < bars - 1
        big = chorus or final
        for t, ch, ln in segments(s['chords'], bar):
            r = bass_root(ch)
            for p in voicing(ch, 55 if big else 52):
                pad.append((t, p, 4 if last else ln, 74 if big else 62))
            tones = sorted(69 + (iv - 9) % 12 for iv in CHORD[ch]) + [81 + (CHORD[ch][0] - 9) % 12]
            if last:
                bass.append((t, r, 3.5, 114))
            elif intro:
                bass.append((t, r, 3.8, 90 if bar < 2 else 100))
                arp += [(t + k * .5, tones[k % 4], .45, 52 if k % 4 == 0 else 42) for k in range(8)]
            elif verse or pre:
                bass += [(t + k * .25, r, .2, 104 if k % 4 == 0 else 84) for k in range(16)]   # the pulse
                if verse:
                    arp += [(t + k * .25, tones[(k * 3) % 4] - 12, .2, 60 if k % 4 == 0 else 46) for k in range(16)]
                else:
                    lead += [(t + o, p, .6, 96) for o in (0, 1.5, 3) for p in voicing(ch, 60)]
            elif brk:
                bass.append((t, r, 3.8, 98))
            else:                                                       # the choruses: octave bass, a pluck sequence
                bass += [(t + k * .5, r + (12 if k % 2 else 0), .42, 114 if k % 2 == 0 else 96) for k in range(8)]
                arp += [(t + k * .25, tones[k % 4], .2, 58 if k % 4 == 0 else 44) for k in range(16)]
                if final:
                    top = voicing(ch, 64)
                    lead += [(t, top[2], 1.9, 62), (t + 2, top[1], 1.9, 58)]
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 122), (b0, CRASH, 112)]
            continue
        if intro:
            if bar >= 2:
                drums += [(b0, KICK, 110), (b0 + 2, KICK, 106)]
            if bar == 3:
                drums += [(b0 + 3 + k * .25, SNARE, 80 + k * 8) for k in range(4)]
            continue
        if verse or pre:
            drums += [(b0, KICK, 116), (b0 + 2, KICK, 112), (b0 + 2.5, KICK, 96), (b0 + 1, SNARE, 112), (b0 + 3, SNARE, 112)]
            drums += [(b0 + k * .5, HAT, 66 if k % 2 == 0 else 50) for k in range(8)]
            if bar == 15:
                drums += [(b0 + 2 + k * .25, SNARE, 84 + k * 5) for k in range(8)]
        elif brk:
            drums += [(b0, KICK, 112)]
            if bar == 27:
                drums += [(b0 + 2 + k * .25, SNARE, 70 + k * 7) for k in range(8)]
        else:                                                           # the choruses
            drums += [(b0 + o, KICK, 118) for o in range(4)] + [(b0 + 1, SNARE, 116), (b0 + 3, SNARE, 116)]
            drums += [(b0 + 1, CLAP, 90), (b0 + 3, CLAP, 90)] + [(b0 + k * .5, HAT, 70 if k % 2 else 56) for k in range(8)]
            if bar in (23, 35):
                drums += [(b0 + 3, TOM_HI, 104), (b0 + 3.25, TOM_MID, 108), (b0 + 3.5, TOM_LO, 112), (b0 + 3.75, TOM_LO, 116)]
        if bar in (4, 12, 16, 20, 24, 28, 32):
            drums.append((b0, CRASH, 104))
    ks = dict(PLAY=[(0, 0), (64, 2), (96, 3), (112, 2)],                   # SYN LEAD, BRASS, GLASS, BRASS
              BASS=[(0, 2), (16, 0), (64, 1), (96, 2), (112, 3)],          # SUB, PULSE, FM BASS, SUB, DIRT BASS
              PAD=[(0, 2), (16, 1), (64, 0), (96, 2), (112, 0)],           # GLASS PAD, STRINGS, SUPERSAW, GLASS PAD, SUPERSAW
              ARP=[(0, 1), (16, 2), (64, 0)],                              # BELL, SQUARE, PLUCK
              LEAD=[(48, 2), (112, 0)],                                    # BRASS LEAD stabs, SAW LEAD
              DRUMS=[(0, 0)])                                              # GATED
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=lead, KEYS=keys, DRUMS=drums, KS=ks)


# DISCO VOLTAGE: French house after Daft Punk (an original song). 123 BPM, A minor with the dorian D major: four on the
# floor, a funky octave bass on the sixteenths, chord stabs on the offbeats, a hook that hops on the sixteenths, disco
# strings and long notes in the breakdown, then the hook again.
DISCO_TEXT = """
0: A4 0 .5 | A4 1 .5 | A4 2 .5 | G4 2.5 .5 | A4 3 1
1: A4 0 .5 | A4 1 .5 | B4 2 .5 | A4 2.5 .5 | E4 3 1
2: A4 0 .5 | C5 .75 .25 | A4 1 .5 | G4 1.5 .5 | E4 2.25 .25 | G4 2.5 .5 | A4 3 1
3: A4 0 .5 | B4 .75 .25 | A4 1 .5 | E4 1.5 .5 | D4 2.25 .25 | E4 2.5 .5 | A4 3 1
4: A4 0 .5 | C5 .75 .25 | A4 1 .5 | G4 1.5 .5 | E4 2.25 .25 | G4 2.5 .5 | A4 3 1
5: A4 0 .5 | B4 .75 .25 | A4 1 .5 | E4 1.5 .5 | D4 2.25 .25 | E4 2.5 .5 | A4 3 1
6: A4 0 .5 | C5 .75 .25 | A4 1 .5 | G4 1.5 .5 | E4 2.25 .25 | G4 2.5 .5 | A4 3 1
7: A4 0 .5 | B4 .75 .25 | A4 1 .5 | E4 1.5 .5 | B4 2 1 | C5 3 .5 | B4 3.5 .5
8: A4 0 .5 | C5 .75 .25 | A4 1 .5 | F4 1.5 .5 | C5 2.25 .25 | A4 2.5 .5 | F4 3 1
9: G4 0 .5 | B4 .75 .25 | G4 1 .5 | D4 1.5 .5 | B4 2.25 .25 | G4 2.5 .5 | A4 3 .5 | B4 3.5 .5
10: C5 0 1 | B4 1 .5 | A4 1.5 .5 | E4 2 .5 | G4 2.5 .5 | A4 3 1
11: A4 0 2 | E4 2 .5 | G4 2.5 .5 | A4 3 1
12: D4+F4 0 .5 | A4 .5 .5 | D4+F4 1.5 .5 | C5 2.5 .5 | A4 3 1
13: E4+G4 0 .5 | B4 .5 .5 | E4+G4 1.5 .5 | B4 2.5 .5 | G4 3 1
14: F4+A4 0 .5 | C5 .5 .5 | F4+A4 1.5 .5 | C5 2.5 .5 | A4 3 1
15: G4+B4 0 .5 | B4 .5 .5 | G4+B4 1.5 .5 | B4 2.5 .5 | G4 3 1
16: A5 0 .5 | C6 .75 .25 | A5 1 .5 | G5 1.5 .5 | E5 2.25 .25 | G5 2.5 .5 | A5 3 .5 | C6 3.5 .5
17: A5 0 .5 | B5 .75 .25 | A5 1 .5 | E5 1.5 .5 | D5 2.25 .25 | E5 2.5 .5 | A5 3 1
18: A5 0 .5 | C6 .75 .25 | A5 1 .5 | G5 1.5 .5 | E5 2.25 .25 | G5 2.5 .5 | A5 3 .5 | C6 3.5 .5
19: B5 0 .5 | A5 .5 .5 | E5 1 .5 | D5 1.5 .5 | E5 2 1 | A5 3 1
20: A5+C6 0 .5 | A5 .75 .25 | F5 1 .5 | C6 1.5 .5 | A5 2.25 .25 | F5 2.5 .5 | A5 3 1
21: G5+B5 0 .5 | G5 .75 .25 | D5 1 .5 | B5 1.5 .5 | G5 2.25 .25 | D5 2.5 .5 | G5 3 1
22: E5+B5 0 1.5 | D5 1.5 .5 | E5 2 1 | B5 3 1
23: B5 0 2 | A5 2 .5 | E5 2.5 .5 | B5 3 1
24: A5 0 2 | C6 2 2
25: A5 0 1 | G5 1 1 | F5 2 2
26: G5 0 2 | B5 2 2
27: B5 0 1 | A5 1 1 | G5 2 2
34: A5+C6 0 1.5 | G5 1.5 .5 | E5 2 .5 | G5 2.5 .5 | A5 3 1
35: A5 0 2 | C6 2 1 | A5 3 1
36: E5+A5 0 3"""


def disco_play():
    """the play track: as written, and the hook of bars 17-22 again in bars 29-34 (the drop)"""
    notes = melody(DISCO_TEXT)
    return notes + [(t + 48, p, ln, v) for t, p, ln, v in notes if 64 <= t < 88]


DISCO_VOLTAGE = dict(
    file='disco-voltage.mid', title='DISCO VOLTAGE', bpm=123,
    chords=['Am', 'D', 'Am', 'D'] + ['Am', 'D', 'Am', 'D', 'F', 'G', 'Am', 'Am'] + ['Dm', 'Em', 'F', 'G']
           + ['Am', 'D', 'Am', 'D', 'F', 'G', 'E', 'E'] + ['F', 'F', 'G', 'G']
           + ['Am', 'D', 'Am', 'D', 'F', 'G', 'Am', 'Am'] + ['Am'],
    phrases=[(2, 3), (6, 7), (10, 11), (14, 15), (18, 19), (22, 23), (30, 31), (34, 35)],
    play=disco_play())


def arrange_disco_voltage(s):
    """French house at 123 BPM: drums and the bass first, the hook over a funky octave bass on the sixteenths and EP
    stabs on the offbeats, a rising run on a pluck arp, the hook up an octave on a supersaw's stabs, a breakdown of
    disco strings and long notes with the kick out, and the drop"""
    bass, pad, arp, keys, lead, drums = [], [], [], [], [], []
    bars = len(s['chords'])
    groove = ((0, 0, .4), (.75, 12, .2), (1.5, 0, .4), (2.25, 0, .2), (2.5, 12, .4), (3.25, 0, .2), (3.5, 7, .4))
    for bar in range(bars):
        last = bar == bars - 1
        intro, a, b = bar < 4, 4 <= bar < 12, 12 <= bar < 16
        c, brk, drop = 16 <= bar < 24, 24 <= bar < 28, 28 <= bar < bars - 1
        for t, ch, ln in segments(s['chords'], bar):
            r = bass_root(ch)
            tones = sorted(69 + (iv - 9) % 12 for iv in CHORD[ch]) + [81 + (CHORD[ch][0] - 9) % 12]
            if last:
                bass.append((t, r, 3, 112))
                pad += [(t, p, 4, 72) for p in voicing(ch, 55)]
                continue
            if brk:
                pad += [(t, p, ln, 76) for p in voicing(ch, 57)]
                bass.append((t, r, ln - .2, 92))
                continue
            bass += [(t + o, r + iv, l, 112 if o in (0, 1.5) else 96) for o, iv, l in groove]   # the funk
            if not intro:
                stab = [(t + o, p, .22, 74) for o in (.5, 1.5, 2.5, 3.5) for p in voicing(ch, 60)]
                (pad if c or drop else keys).extend(stab)
            if b:
                arp += [(t + k * .25, tones[k % 4], .2, 56 if k % 4 == 0 else 44) for k in range(16)]
        b0 = bar * 4
        if last:
            drums += [(b0, KICK, 122), (b0, CRASH, 110)]
            continue
        if brk:
            drums += [(b0 + k * .5 + .5, HAT, 54) for k in range(0, 8, 2)]
            if bar == 27:
                drums += [(b0 + k * .25, SNARE, 60 + k * 4) for k in range(16)]
            continue
        drums += [(b0 + o, KICK, 118) for o in range(4)] + [(b0 + o + .5, OHAT, 74) for o in range(4)]
        drums += [(b0 + k * .25, HAT, 56 if k % 2 == 0 else 38) for k in range(16)]
        if not (intro and bar < 2):
            drums += [(b0 + 1, CLAP, 104), (b0 + 3, CLAP, 104), (b0 + 1, SNARE, 84), (b0 + 3, SNARE, 84)]
        if bar in (4, 16, 28):
            drums.append((b0, CRASH, 104))
    ks = dict(PLAY=[(0, 0), (64, 4), (96, 3), (112, 4)],                   # SYN LEAD, FUZZ, GLASS (the breakdown), FUZZ
              BASS=[(0, 1)],                                               # FM BASS
              PAD=[(0, 0), (96, 1), (112, 0)],                             # SUPERSAW stabs, STRINGS, SUPERSAW
              ARP=[(48, 0)],                                               # PLUCK
              KEYS=[(16, 0)],                                              # FM EP
              DRUMS=[(0, 1)])                                              # TIGHT
    return dict(BASS=bass, PAD=pad, ARP=arp, LEAD=lead, KEYS=keys, DRUMS=drums, KS=ks)


# ---------------------------------------------------------------- a standard MIDI file, format 1

def vlq(n):
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.append((n & 0x7F) | 0x80)
        n >>= 7
    return bytes(reversed(out))


def chunk(tag, data):
    return tag + struct.pack('>I', len(data)) + data


def mtrk(name, events):
    """events: (tick, order, bytes); note-offs (order 0) before CCs (1) before note-ons (2) on the same tick"""
    evs = [(0, -1, b'\xff\x03' + vlq(len(name)) + name.encode('ascii'))] + sorted(events, key=lambda e: (e[0], e[1]))
    out, last = b'', 0
    for tick, _, data in evs:
        out += vlq(tick - last) + data
        last = tick
    return chunk(b'MTrk', out + b'\x00\xff\x2f\x00')


def note_events(ch, notes, drum=False):
    evs = []
    for t, p, ln, v in notes:
        on, off = round(t * PPQ), round((t + (ln if not drum else .125)) * PPQ)
        evs += [(on, 2, bytes([0x90 | ch, p, v])), (off, 0, bytes([0x80 | ch, p, 0]))]
    return evs


def keyswitch_events(ch, switches):
    """a keyswitch is a short note 0-11, sorted before the notes on the same tick"""
    evs = []
    for beat, index in switches:
        tick = round(beat * PPQ)
        evs += [(tick, 1, bytes([0x90 | ch, index, 100])), (tick + PPQ // 8, 0, bytes([0x80 | ch, index, 0]))]
    return evs


def write_song(s, parts):
    us = round(60_000_000 / s['bpm'])
    conductor = [(0, 0, b'\xff\x51\x03' + us.to_bytes(3, 'big')), (0, 0, b'\xff\x58\x04\x04\x02\x18\x08')]
    if s.get('level'):                    # (D13: the level comes from the notes; a LEVEL marker only to override them)
        mark = ('LEVEL ' + s['level']).encode('ascii')
        conductor.append((0, 0, b'\xff\x06' + vlq(len(mark)) + mark))
    tracks = [mtrk(s['title'], conductor)]
    play = note_events(CH['PLAY'], s['play'] if isinstance(s['play'], list) else melody(s['play']))
    play += note_events(CH['PLAY'], [(a * 4, OP_NOTE, (b - a + 1) * 4, 100) for a, b in s['phrases']])   # OP phrases: B-1, held
    play += note_events(CH['PLAY'], [(a * 4, SOLO_NOTE, (b - a + 1) * 4, 100) for a, b in s.get('solos', [])])   # FREE SOLO: A#-1, held
    ks = parts.get('KS', {})
    tracks.append(mtrk('PLAY', play + keyswitch_events(CH['PLAY'], ks.get('PLAY', []))))
    for name in ('BASS', 'PAD', 'ARP', 'LEAD', 'KEYS', 'DRUMS'):
        if parts.get(name):
            drum = name == 'DRUMS'
            notes = [(t, p, .125, v) for t, p, v in parts[name]] if drum else parts[name]
            tracks.append(mtrk(name, note_events(CH[name], notes, drum) + keyswitch_events(CH[name], ks.get(name, []))))
    return chunk(b'MThd', struct.pack('>HHH', 1, len(tracks), PPQ)) + b''.join(tracks)


def main():
    os.makedirs(os.path.join(HERE, 'songs'), exist_ok=True)
    files = []
    for s, arrange in ((FIRST_LIGHT, arrange_first_light), (NEON_CARRIER, arrange_neon_carrier),
                       (ENGINE_TOUR, arrange_engine_tour), (CHROME_HORIZON, arrange_chrome_horizon),
                       (KIRAMEKI_RUSH, arrange_kirameki_rush), (YOZORA_SPARK, arrange_yozora_spark),
                       (TURBO_NOIR, arrange_turbo_noir), (DISCO_VOLTAGE, arrange_disco_voltage)):
        data = write_song(s, arrange(s))
        with open(os.path.join(HERE, 'songs', s['file']), 'wb') as f:
            f.write(data)
        files.append({'file': s['file'], 'data': base64.b64encode(data).decode()})
        print(f"songs/{s['file']}: {len(data)} bytes")
    ref = os.path.join(HERE, 'reference', 'vavehero.html')
    html = open(ref, encoding='utf-8').read()
    block = '/*@SONGS*/window.VAVEHERO_SONGS = ' + json.dumps(files) + ';/*SONGS@*/'
    new, n = re.subn(r'/\*@SONGS\*/.*?/\*SONGS@\*/', lambda m: block, html, flags=re.S)
    if n != 1:
        raise SystemExit('reference/vavehero.html has no songs block (/*@SONGS*/ ... /*SONGS@*/)')
    if new != html:
        open(ref, 'w', encoding='utf-8').write(new)
        print('reference/vavehero.html: songs block updated')


if __name__ == '__main__':
    main()
