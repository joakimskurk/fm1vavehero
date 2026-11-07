#!/usr/bin/env python3
"""The built-in songs, written as standard MIDI files exactly as docs/songs.md asks of a song made in a DAW, and
embedded in reference/keyfall.html so the reference stays one file. The reference imports them with the same code
as a file you load yourself.

    python3 tools/songs.py            # writes songs/*.mid and the songs block in reference/keyfall.html

Channels (1-based, as a DAW shows them): 1 PLAY (the play track), 2 BASS, 3 PAD, 4 ARP, 5 LEAD, 6 KEYS,
10 DRUMS (General MIDI notes). Channel 16 belongs to LINK. CC 85 on channel 1 marks an OP phrase (127 on, 0 off).
Notes 0-11 (C-1..B-1) on a channel are keyswitches: they pick the channel's patch, note 0 the first (D6).
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
CHORD = {'C': [0, 4, 7], 'G': [7, 11, 14], 'Am': [9, 12, 16], 'F': [5, 9, 12], 'Em': [4, 7, 11], 'Dm': [2, 5, 9]}
ROOT = {'C': 0, 'G': 7, 'Am': 9, 'F': 5, 'Em': 4, 'Dm': 2}
KICK, SNARE, CLAP, HAT, OHAT, CRASH, TOM_LO, TOM_MID, TOM_HI = 36, 38, 39, 42, 46, 49, 45, 48, 50


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
    tracks = [mtrk(s['title'], conductor)]
    play = note_events(CH['PLAY'], melody(s['play']))
    for a, b in s['phrases']:                                           # OP phrases: CC 85 on channel 1
        play += [(a * 4 * PPQ, 1, bytes([0xB0, 85, 127])), ((b + 1) * 4 * PPQ, 1, bytes([0xB0, 85, 0]))]
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
    for s, arrange in ((FIRST_LIGHT, arrange_first_light), (NEON_CARRIER, arrange_neon_carrier)):
        data = write_song(s, arrange(s))
        with open(os.path.join(HERE, 'songs', s['file']), 'wb') as f:
            f.write(data)
        files.append({'file': s['file'], 'data': base64.b64encode(data).decode()})
        print(f"songs/{s['file']}: {len(data)} bytes")
    ref = os.path.join(HERE, 'reference', 'keyfall.html')
    html = open(ref, encoding='utf-8').read()
    block = '/*@SONGS*/window.KEYFALL_SONGS = ' + json.dumps(files) + ';/*SONGS@*/'
    new, n = re.subn(r'/\*@SONGS\*/.*?/\*SONGS@\*/', lambda m: block, html, flags=re.S)
    if n != 1:
        raise SystemExit('reference/keyfall.html has no songs block (/*@SONGS*/ ... /*SONGS@*/)')
    if new != html:
        open(ref, 'w', encoding='utf-8').write(new)
        print('reference/keyfall.html: songs block updated')


if __name__ == '__main__':
    main()
