#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# CI: fm1-emu (joakimeriksson/fm1-emu, pinned) built from source, then the engine's load on the package just built
# (tools/emu_cpu.sh) and its timing, block by block and frame by frame (tools/emu_timing.py: the DEMO and a MIDI flood),
# and the settings saved on the newer FM-1s' flash chip. The numbers land in the job's log.
set -e
REV=c7e47c4fcbdaafb4beffa40493da1b2b07ffa263   # (0.5.1: the newer FM-1s' flash chip, --flash-chip uc25hq80)
DIR="$HOME/fm1-emu"
if [ ! -x "$DIR/target/release/fm1-emu" ]; then
    rm -rf "$DIR"
    git init -q "$DIR"
    git -C "$DIR" fetch -q --depth 1 https://github.com/joakimeriksson/fm1-emu "$REV"
    git -C "$DIR" checkout -q FETCH_HEAD
    echo "emu_ci: building fm1-emu ($REV)"
    (cd "$DIR" && timeout 900 cargo build -q --release -p fm1-emu)
fi
echo "emu_ci: fm1-emu ready"
EMU_MAX=${EMU_MAX:-45} timeout 1200 sh tools/emu_cpu.sh build/vavehero.fwsc   # (45 %: ROADMAP's ceiling)
FM1_EMU="$DIR/target/release/fm1-emu" FM1_LCD_TIMED=1 timeout 1500 python3 tools/emu_timing.py build/vavehero.fwsc build/vavehero.dis --ci   # (each audio block, each frame, the screen at its own speed)
# the newer FM-1s' flash chip (the UC25HQ80, JEDEC 0xB36014; Dinghy 0.5): the settings are saved on it too. The song
# list, GLO (a setting changed), 2 s of quiet: a sector erased and written
FM1_FLASH_TRACE=1 timeout 600 "$DIR/target/release/fm1-emu" --fwsc build/vavehero.fwsc --flash-chip uc25hq80 --ms 6000 \
    --keys '800:PLAY,1500:GLO' --quiet > build/emu-uc25hq80.log 2>&1
if grep -q "flash cmd 20 " build/emu-uc25hq80.log && grep -q "flash cmd 02 " build/emu-uc25hq80.log; then
    echo "emu_ci: the newer FM-1s' flash chip (UC25HQ80): the settings saved"
else
    echo "emu_ci: the newer FM-1s' flash chip (UC25HQ80): the settings were NOT saved"; exit 1
fi
