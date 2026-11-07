#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# KEYFALL's own host tests (no FM-1, no JieLi toolchain): the app compiles cleanly for the host, and the default
# script plays it. More come with each step of the port (ROADMAP.md, phase 3). Dinghy's own tests:
# tests/run_tests.sh (after ./build.sh).
set -e
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"
fail=0
run() { echo "== $1"; shift; "$@" || fail=1; }

run "keyfall.c compiles for the host without a warning (-Werror)" \
    cc -O2 -Wall -Werror -Wno-unused-function -DDINGHY_HOST -DAPP_VER='"test"' -Ifirmware/hal -Ifirmware/src -Ifirmware/gen \
       -o "$OUT/keyfall-werror" firmware/src/keyfall.c
run "the app on the host: the default script, screens and sound" sh tests/run_host.sh
run "the sound file is there and not empty" test -s "$OUT/keyfall.wav"

[ $fail = 0 ] && echo "run_keyfall: all passed" || { echo "run_keyfall: FAILED"; exit 1; }
