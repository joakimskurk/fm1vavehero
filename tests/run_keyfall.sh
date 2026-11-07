#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# KEYFALL's own host tests (no FM-1, no JieLi toolchain): the app compiles cleanly for the host and plays, the
# engine against the reference (every patch's level and brightness, tests/kf/parity.py), the worst case renders
# without clipping, the tables are what the reference makes. Dinghy's own tests: tests/run_tests.sh (after
# ./build.sh). The engine's cost on the chip: tools/emu_cpu.sh (fm1-emu, CI).
set -e
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT" build/audio
fail=0
run() { echo "== $1"; shift; "$@" || fail=1; }

if command -v node >/dev/null 2>&1; then
    run "firmware/gen/kf_tables.h is what tools/gen_kf_tables.mjs makes from the reference" node tools/gen_kf_tables.mjs --check
    run "firmware/gen/kf_songs.h is what the reference's import makes of songs/ (tools/mid2keyfall.mjs)" node tools/mid2keyfall.mjs --check
fi
run "keyfall.c compiles for the host without a warning (-Werror)" \
    cc -O2 -Wall -Werror -Wno-unused-function -DDINGHY_HOST -DAPP_VER='"test"' -Ifirmware/hal -Ifirmware/src -Ifirmware/gen \
       -o "$OUT/keyfall-werror" firmware/src/keyfall.c
run "the engine alone compiles without a warning (-Wextra -Werror)" \
    cc -O2 -Wall -Wextra -Werror -Wno-unused-function -Ifirmware/src -Ifirmware/gen -o build/kf_render tests/kf/kf_render.c -lm
run "the engine tour: every patch and kit" sh -c "build/kf_render tour build/audio/kf-tour.wav build/audio/kf-tour.json >/dev/null"
run "the engine against the reference: levels within 1.5 dB, brightness within 25 %" python3 tests/kf/parity.py build/audio/kf-tour.json tests/kf/ref-tour.json
for i in 0 1 2 3; do
    run "song $i, its first 30 s against the reference's renderSong: every 4 bars within 1.5 dB" sh -c \
        "build/kf_render song $i 30 build/audio/kf-song$i.wav build/audio/kf-song$i.json >/dev/null && python3 tests/kf/parity.py build/audio/kf-song$i.json tests/kf/ref-song$i.json -q"
done
run "the worst case (kf_bench.h: ENGINE TOUR from bar 41) renders" build/kf_render stress build/audio/kf-stress.wav
run "the worst case does not clip" python3 - <<'PY'
import struct, sys
d = open("build/audio/kf-stress.wav", "rb").read()[44:]
x = struct.unpack("<%dh" % (len(d) // 2), d)
peak = max(abs(v) for v in x) / 32768
print("  peak %.3f" % peak)
sys.exit(0 if peak < 0.999 else 1)
PY
run "the app on the host: the default script" sh tests/run_host.sh
run "the sound check: keys, the song channels with a keyswitch, the drums, the DEMO" sh tests/run_host.sh tests/kf/sound.txt
run "the sound file is there and not empty" test -s "$OUT/keyfall.wav"

[ $fail = 0 ] && echo "run_keyfall: all passed" || { echo "run_keyfall: FAILED"; exit 1; }
