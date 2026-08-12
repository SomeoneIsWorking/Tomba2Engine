---
id: C043
kind: claim
status: holds
created: 2026-08-12
tags: producers,render
depends: game/render/perobj_dispatch.cpp#resolvePerModeEmitter, game/render/render_walk.cpp
---

## Claim

Guest 0x800803DC (the substrate generic GT3/GT4 packet emitter) is STILL EARNABLE by the present build as Render::perObjFlush's producer key — it is NOT a fossil. Render::resolvePerModeEmitter returns it whenever the render-mode byte 0x800BF870 selects MODE_TABLE's generic label 0x8003F788, which 12 of the 22 entries do (modes 3, 9-19). The seaside/mode-0 corpus every boot gate and replay uses routes to 0x80146478 instead, which is the ONLY reason 119 historical native legs never earned it. Corollary: kanban #91's suspicion that the claim was dead is REFUTED; the defect is the claim file's missing PROVENANCE, not the address.

## Evidence

NINE legs were LAUNCHED and all nine are disclosed here — an earlier version of this evidence named five, and an omitted leg is a silently-dropped negative. Each ran with its own PSXPORT_PRODUCERS_DIR and a nonexistent PSXPORT_PRODUCERS_DB, so loadClaims warns "claim set NOT loaded" and the written claims.txt is EXACTLY that leg's EARNED set. Eight wrote a run file: k91/producers (boot 400f) 6 earned, k91/hut (hut-entry-door-freeze replay) 15, k91/hutalt (the same script at 700 frames, a REPLICATE — undisclosed before, and it is the file that survived the union collision) 15, k91/pad3 21, k91/pad4 23, all WITHOUT 0x800803DC; k91/warp9 8 earned WITH it, replicated by k91/warp12 (9) and k91/warp19 (9). The NINTH, k91/warp3, CRASHED and wrote nothing: exit=139 at 77.0s, rec_dispatch_miss -> abort on '[recomp-MISS 0] no recompiled fn for 0x801127EC (caller ra=0xDEAD0000, a0=0x800E7E80, c->pc=0x8001DC40)', overlay slot A03 (scratch/k91/warp3.gate.log). Mode 3 is one of the 12 generic-label modes, so warp3 would have been a fourth replication — it is UNMEASURED, not a negative.

The warp9 leg is the proof because it holds both outcomes of the one switch: scratch/logs/gate-run-20260812-152930.log lines 100/119/121 — 'bf870=9', then 0x80146478 perObjFlush 34668 prims f130..f227 (mode 0) and 0x800803DC perObjFlush 110124 prims f230..f527 (mode 9). MODE_TABLE read statically from scratch/bin/tomba2/MAIN.EXE file offset 0x5A68 (text at 0x80010000): 12 of 22 entries are 0x8003F788.

BUILD IDENTITY, which the first version of this evidence asserted on an assumption it did not state. "at HEAD 11a75fb, build confirmed up to date, no source newer than the binary" is an MTIME argument: it infers that the binary those legs ran was built from that code because no source file was newer than it. That is the exact unstated assumption tools/producers.py stale was fixed for (a run of a stale or dirty binary reads re-earned). The binary has since been rebuilt (md5 4e21d13225122177bcf1945378197d2b, 16:45:49), so those eight legs can no longer be tied to any binary at all and `stale` correctly reports them as 0 credited / 10 BUILD PROVENANCE UNKNOWN, exit 1. THE POSITIVE WAS THEREFORE RE-RUN under a recorded identity: scratch/k91c/warp9 ('newgame / run 200 / warp 9 / run 300', run-2026-08-12T17:27:01.jsonl) and scratch/k91c/hut (the hut-entry replay, run-2026-08-12T17:28:45.jsonl), each with a tools/gate.py binary.txt recording md5 4e21d132… before AND after the run and naming which run file it covers. 0x800803DC is earned on the warp9 re-run and absent from the hut re-run, reproducing the switch on a binary of provable identity; `producers.py stale --obs scratch/k91c/warp9 --obs scratch/k91c/hut` reads 10/10 RE-EARNED, exit 0.

## Note on this claim reading STALE in `info.py claim check`

FALSE STALE, and the cause is mechanical rather than substantive: this claim file is UNTRACKED, so claim_baseline falls back to `created:` at DAY resolution (2026-08-12T00:00) instead of the +1s add-commit anchor. Every same-day commit to its deps then counts as "since" — it names 9c94008 (00:12) and bc2a31c (02:09), both of which PRECEDE the 15:23 measurement, so the evidence rests on code that has not moved. It will read fresh the moment the operator commits the file (add-commit anchor > all four named commits). Do NOT `claim confirm` it to silence this — a date-only `verified_at:` lands at the same midnight and would not change the result. The real fix is in tools/info.py's claim_baseline: parse a FULL ISO timestamp in `verified_at:`/`reconfirmed:` rather than truncating to `v[:10]`, so a same-day re-verification can discriminate same-day commits (handover — that file was not in this session's file set).

## What would falsify it

if MODE_TABLE (0x80015268) is not fixed game DATA but is rewritten at runtime, or if a change to Render::resolvePerModeEmitter stops returning GENERIC_EMITTER for a generic-label mode, the reachability argument collapses — re-run 'warp 9' and check the earned set
