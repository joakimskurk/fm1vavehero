#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The firmware's engine against the reference's: the engine tour's levels and brightness, patch by patch
# (tests/kf/kf_render.c against tools/ref_tour.cjs; the reference's numbers are kept in tests/kf/ref-tour.json).
#   python3 tests/kf/parity.py C.json [REF.json]     exit 1 if a patch is off by more than the tolerances
import json, sys
TOL_DB, TOL_BRIGHT = 1.5, 0.25
c = {r["name"]: r for r in json.load(open(sys.argv[1]))}
ref = json.load(open(sys.argv[2] if len(sys.argv) > 2 else "tests/kf/ref-tour.json"))
bad = 0
print(f"  {'':12} {'C dB':>7} {'ref dB':>7} {'diff':>6}   {'bright C/ref':>12}")
for r in ref:
    m = c.get(r["name"])
    if not m:
        print(f"  {r['name']:12} missing"); bad += 1; continue
    d = m["rms_db"] - r["rms_db"]
    br = m["bright"] / r["bright"] if r["bright"] else 1
    ok = abs(d) <= TOL_DB and abs(br - 1) <= TOL_BRIGHT
    bad += not ok
    print(f"  {r['name']:12} {m['rms_db']:7.2f} {r['rms_db']:7.2f} {d:+6.2f}   {br:12.2f}{'' if ok else '   <-- off'}")
print(f"parity: {len(ref) - bad} of {len(ref)} within {TOL_DB} dB and {int(TOL_BRIGHT * 100)} % brightness")
sys.exit(1 if bad else 0)
