---
id: I048
kind: instrument
status: trusted
created: 2026-08-12
---

## Instrument

Per-leg earned-claim measurement: run the game with PSXPORT_PRODUCERS_DIR=<own dir> and PSXPORT_PRODUCERS_DB=<nonexistent path>, then read that dir's claims.txt as EXACTLY the addresses that leg earned. This is the only honest way to get an earned-set out of the present runtime, because ProducerCensus::appendClaims writes the LOADED set unioned with the earned one — the shared scratch/producers/claims.txt re-emits its own contents every run, so its newest block always looks freshly earned and cannot answer 'what did this run earn'.

## Validated by

BOTH CLASSES on the same build (HEAD 11a75fb). NEGATIVE: 4 seaside/mode-0 legs (boot, hut-entry, pad_session.3, pad_session.4) wrote 6/15/21/23 addresses, none including 0x800803DC, while the shared claims.txt contains it — so the method can withhold an address the shared file asserts. POSITIVE: 'newgame / run 200 / warp 9 / run 300' wrote 8 including 0x800803DC, matching the run-end census lines in scratch/logs/gate-run-20260812-152930.log (0x800803DC native 110124), and 'loadClaims: claim set NOT loaded' is printed on every leg so an empty load can never pass for an empty earn. Replicated warp 12, warp 19.

## Known failure modes

- **IT MEASURES A LEG, NOT A BUILD.** The earned set says what the binary that ran earned; it carries no identity for that binary, so "HEAD 11a75fb" in the validation above is an inference from mtime (no source newer than the binary), not a measurement. The same assumption is what tools/producers.py stale was fixed for. Pair this method with tools/gate.py, which writes a binary.txt recording the binary's md5 BEFORE and AFTER the run plus the run files it covers; without that, `stale` can only report BUILD PROVENANCE UNKNOWN — measured: the eight 15:23-15:33 legs in scratch/k91/* read 0 credited / 10 UNKNOWN once the binary was rebuilt at 16:45:49.
- **A NEGATIVE HERE IS NOT-EARNED, NEVER DEAD.** A producer key is a function of guest state, so an address is absent from a leg's earned set whenever the leg never visited the content that earns it (0x800803DC is absent from all five mode-0 legs and present on warp 9/12/19). Reach the content before concluding anything about the address.
- **A CRASHED LEG WRITES NOTHING AND LOOKS LIKE NOTHING.** A leg dir with no run file is indistinguishable from a leg never launched unless the gate log is kept beside it — scratch/k91/warp3 aborted on a recomp miss after 228 frames and silently contributed zero. Keep the per-leg .gate.log, and read the corpus with `producers.py stale --obs <legdir>` (repeatable), which prints an empty leg dir as a dropped negative.
- **NEVER `cp` SEVERAL LEGS INTO ONE DIRECTORY.** The runtime names run files `run-<stamp>.jsonl` at ONE-SECOND resolution: hutalt and warp9 both finished 15:30:49 and the copy silently lost warp9, the leg the positive rests on. Read legs where they lie.
