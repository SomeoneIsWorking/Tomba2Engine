---
id: C034
kind: claim
status: holds
created: 2026-08-06
tags: 
depends: game/render/fx_line.cpp, external/psxport/runtime/psx/gpu_native.cpp
---

## Claim

The GP0 LINE class has no fourth emitter: across the WHOLE 16-replay library, every line packet the guest emits comes from exactly three addresses — 0x8013E9D8, 0x80122974, 0x8013E08C — and all three have a LIVE native producer in game/render/fx_line.cpp

## Evidence

lineprim+otattr census, psx_render leg (the only leg that emits GP0 lines; a pc leg can only report 0 and would be a silent instrument), PSXPORT_GATE=1 headless, all 16 .pad files under replays/. 97,516 line packets total, UNATTRIBUTED = 0 (every packet resolved to an emitter fn via the otattr store-span table). Per-replay counts in scratch/lineclass/logs/*.err; harness scratch/lineclass/census.sh. Harness validated against the recorded figure first: bucket-softlock reproduced 1658 packets / 1064 op-0x4A + 594 op-0x5E exactly. This is the falsifier C031 stated and nobody ran ('a lineprim census in another scene showing a line packet from a fourth emitter'); it is now run over 16 scenes instead of 1.

## What would falsify it

a lineprim census that resolves a line packet to a fourth emitter address, OR any packet with fn=0x00000000 (an unattributed packet means the census has a blind spot it did not have here). Blind spots that would hide one: 4 replays were frame-capped at 6000 (dark-screen-repro 61030f, save-prompt-black-screen 12500f, seesaw-weight 6668f, sequence-softlock-2 7049f), and the library does not cover all 22 areas — this is NOT the docs/areas.md 22-area sweep.
