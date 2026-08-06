---
id: I038
kind: instrument
status: trusted
created: 2026-08-06
---

## Instrument

PSXPORT_PRESENT_SHOT_AT + tools ppmdiff over two SEPARATELY-BUILT binaries (producer ON vs producer deleted) as the pixel gate for a native render producer

## Validated by

VALIDATED BOTH WAYS on real data, 2026-08-06, Tomba2 bucket-softlock.pad, headless, PSXPORT_GATE=1 pc_render. POSITIVE: leg A (rope producers live) vs leg B (worldLineDraw + shockwaveRingRender deleted) differ by 1584/1197/927 px at presents 440/445/450, bbox 41x263 / 41x206 / 29x164 — tall, narrow, tracking right as the camera pans, i.e. rope-shaped. NEGATIVE: the same instrument, same mode, reports exactly 0 changed pixels of 691200 on the presents where the producer is not called (f455+; the ropeline frame stamp shows the last call at f453). SENSITIVITY: a third leg with a 12x-wider pure-white stroke gives 19107 px = 12.06x leg A's 1584, and the bbox widens 41->107 px = +66 = 22px stroke x ires 3 — the response scales as the geometry predicts, so the instrument is measuring the stroke and not an artefact. scratch/lineclass/ppmdiff.py carries --selftest (proves it can report both a zero and a nonzero) and REFUSES an empty corpus rather than printing a clean zero.

## Known failure modes

(none recorded yet)
