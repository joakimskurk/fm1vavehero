# PLATFORM

KEYFALL runs on **Dinghy 0.5**, imported as source (commit `1444aa4` of this repo, unchanged).

- upstream: https://github.com/zvenson/dinghy
- commit: `ea1b6bf` (2026-11, after "Dinghy 0.5" `1675934`; the release workflow's file pattern only). Dinghy 0.5
  is 0.4 plus `f77bf03`: `firmware/hal/fm1_flash_map.h` accepts the UC25HQ80 (`0xB36014`) of FM-1s sold since
  autumn 2026 beside the Puya `0x856014`.
- Dinghy's docs stay in `docs/` (`API.md`, `RULES.md`, `MAKE-IT-YOURS.md`, `SMK37.md`). Its README is
  `docs/DINGHY_README.md` (the toolchain, the SDK, the host mode).

## What differs from upstream

Kept small, so a later Dinghy can be merged by diffing these files.

| file | change |
| --- | --- |
| `firmware/src/dinghy.c` | renamed `keyfall.c`: the app; `APP_NAME "KEYFALL"`, `STORE_MAGIC` "KFAL" |
| `tools/build.py` | compiles `keyfall.c`; outputs `keyfall.*`; identity `FM-1_95N` (N = the minor version, 0 = a dev build; 95N is free in Dinghy's list); loader screen "KEYFALL" in cyan `0x5F1F`; data family `KEYFALL` |
| `build.sh` | a comment |
| `tests/run_host.sh` | builds `keyfall.c` as `build/host/keyfall` |
| `tests/run_tests.sh` | `keyfall.*` names; the SMK-37 packages' identities `_95N` |
| `tests/install_test.py` | `keyfall.*` names |
| `README.md` | moved to `docs/DINGHY_README.md`; KEYFALL's README in its place |
| `.gitignore` | KEYFALL's `build/` line kept, Dinghy's lines added |
| `.github/workflows/release.yml` | replaced by `build.yml` (zvenson/fm1-build, as RoboSeq and SkurkyMello) |

Nothing in `firmware/hal/`, `firmware/loader/`, `firmware/app.ld`, `platform.c` or `tests/host.c` is changed.

## Updating Dinghy

1. Diff the new upstream against the commit above for the imported paths.
2. Apply upstream changes; re-apply the table above.
3. `make test`, CI's build, a boot in fm1-emu.
4. Update the commit here.
