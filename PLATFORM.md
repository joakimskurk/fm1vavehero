# PLATFORM

VaveHero runs on **Dinghy 0.6.1**'s platform, imported as source (0.5 in commit `1444aa4` of this repo, 0.6.1's
platform files on 2026-11-27).

- upstream: https://github.com/zvenson/dinghy
- commit: `14cda7a` (2026-11-26, the tag `v0.6.1` plus its screen note). What VaveHero takes from it:
  - **0.6.1, the newer FM-1s' screen** (`firmware/src/lcd.c`): M-VAVE's own ST7789V register values in M-VAVE's
    order (porch, gate, VCOM, supplies, 60 Hz, both gammas, one data line, full brightness), found by Jake Handy for
    h1. Without them the units with the FM-1 / M-VAVE logo under the screen show washed-out colours, no reds, or
    nothing. `docs/NEWER-FM1-SCREEN.md` is upstream's note on it.
  - 0.6's platform hooks: `MIDI_RT_HOOK` in `usb.c`, `midi_uart.c` and `tests/host.c` (clock and transport where they
    arrive) and the `LCD_MIRROR` hooks in `lcd.c`. Both do nothing unless the app defines them; VaveHero does not.
  - 0.5 (in since the start): `firmware/hal/fm1_flash_map.h` accepts the UC25HQ80 (`0xB36014`) of FM-1s sold since
    autumn 2026 beside the Puya `0x856014`.
- Not taken from 0.6: the kit (`kit_clock.h`, `kit_midi.h`, `kit_backup.h`, `kit_link.h`, `kit_mirror.h`,
  `screen_mirror/`) and its tests, and `dinghy.c`'s new GLOBAL page. They are headers Dinghy's own app includes;
  VaveHero has its own MIDI MODE and settings and does not use them.
- Dinghy's docs stay in `docs/` (`API.md`, `RULES.md`, `MAKE-IT-YOURS.md`, `SMK37.md`). Its README is
  `docs/DINGHY_README.md` (the toolchain, the SDK, the host mode).

## What differs from upstream

Kept small, so a later Dinghy can be merged by diffing these files.

| file | change |
| --- | --- |
| `firmware/src/dinghy.c` | renamed `vavehero.c`: the app; `APP_NAME "VAVEHERO"`, `STORE_MAGIC` "VHRO" |
| `tools/build.py` | compiles `vavehero.c`; outputs `vavehero.*`; identity `FM-1_95N` (N = the minor version, 0 = a dev build; 95N is free in Dinghy's list); loader screen "VAVEHERO" in cyan `0x5F1F`; data family `VHERO` |
| `build.sh` | a comment |
| `tests/run_host.sh` | builds `vavehero.c` as `build/host/vavehero` |
| `tests/run_tests.sh` | `vavehero.*` names; the SMK-37 packages' identities `_95N` |
| `tests/install_test.py` | `vavehero.*` names |
| `README.md` | moved to `docs/DINGHY_README.md`; VaveHero's README in its place |
| `.gitignore` | VaveHero's `build/` line kept, Dinghy's lines added |
| `LICENSING.md` | VaveHero's parts and their author, VaveHero's logo, and the Inter font of its letters added |
| `.github/workflows/release.yml` | replaced by `build.yml` (zvenson/fm1-build, as RoboSeq and SkurkyMello) |
| `firmware/src/usb.c` | `usb_retry` waits the 25 ms between `fm1_usb_reset` and the attach without blocking (`usb_attach_after_reset`); boot and recovery keep `usb_start`, which waits at once. Without a USB host (a power bank, a charger) the busy wait stalled the main loop for ~30 ms every second, measured in fm1-emu: frames skipped, key presses stamped late |
| `firmware/hal/fm1_input.h` | `FM1_NOTE_EDGE(key, down)`, called in the scan (the timer ISR) as a note key's edge is debounced; a no-op unless the app defines it. VaveHero stamps each press and release with it |
| `tests/host.c` | calls `FM1_NOTE_EDGE` for each key a script moves, as the FM-1's scan does |

Nothing else in `firmware/hal/`, `firmware/loader/`, `firmware/app.ld`, `firmware/src/` or `tests/` is changed. Checked
file by file against upstream `14cda7a` (0.6.1) on 2026-11-27: `firmware/hal/`, `firmware/loader/`, `firmware/app.ld`,
`lcd.c` and `midi_uart.c` are upstream's; `usb.c` and `tests/host.c` are upstream's with the changes in this table; the
kit files above are not here.

## Updating Dinghy

1. Diff the new upstream against the commit above for the imported paths.
2. Apply upstream changes; re-apply the table above.
3. `make test`, CI's build, a boot in fm1-emu.
4. Update the commit here.
