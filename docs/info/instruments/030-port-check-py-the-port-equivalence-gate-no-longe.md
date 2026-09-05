---
id: I030
kind: instrument
status: trusted
created: 2026-07-30
---

## Instrument

dynamic differential evidence — the port equivalence gate, no longer passes vacuously

## Validated by

Was a GATE THAT CERTIFIED ON NOTHING: a missing path was warning-and-continue, and an empty marker set printed 'nothing to check' and exited 0. Because port_check takes FILES while every neighbouring tool (binary ABI evidence, direct executable disassembly, codemap.py --addr) takes an ADDRESS, the natural mistake 'dynamic differential evidence 800834A0' printed a warning and EXITED 0 — a green gate on input never read. Now both paths exit 2. Validated on BOTH classes on the live tree: NEGATIVE 1 'dynamic differential evidence 800834A0' -> 'REFUSING - 1 of 1 given path(s) do not exist' + 'that looks like a GUEST ADDRESS', exit 2. NEGATIVE 2 'dynamic differential evidence game/core/game_config.cpp' (real file, no markers) -> 'NOTHING CHECKED - read 1 file(s) and found 0 markers. THIS IS NOT A PASS', exit 2. POSITIVE 'dynamic differential evidence game/render/perobj_dispatch.cpp game/audio/sequencer.cpp' still reports 4 PASS / 1 FAIL / 4 UNPROVABLE of 9 methods. POSITIVE 2 '--all' unaffected: 101 PASS / 20 FAIL / 16 UNPROVABLE of 137 methods checked, exit 1. Not wired into .git/hooks/pre-commit, so the exit-code change breaks no gate.

## Known failure modes

(none recorded yet)
