---
id: I049
kind: instrument
status: trusted
created: 2026-08-12
---

## Instrument

tools/producers.py stale — reconstructs per-claim provenance from the run-*.jsonl corpus (a row with prims_native>0 is an EARN event; the per-run JSONL is NOT union-echoed the way claims.txt is), dates each claim against the newest commit touching the paths that DECIDE a producer key, and credits an earn ONLY to a leg whose RECORDED binary md5 equals the present build's. Read each leg where it lies (`--obs <legdir>`, repeatable) — never cp several legs into one directory.

## Validated by

--selftest drives THE SHIPPING PATH (`run_stale`, git-derived reference and build gate included) over fixture git repos with controlled commit dates and fixture binaries: 'producers stale --selftest: 15 class(es) gated over the SHIPPING path, 0 FAIL(s)' (exit 0). Classes: re-earned / fossil / guest-only-never-earned / unparseable-line count / docs-only-HEAD-must-not-fossilise / dirty-key-path / binary-older-than-key-code / legacy-leg-certifies-nothing / binary-swapped-mid-run / build-provenance-UNKNOWN third state / two-runs-one-identity / legacy-identity-over-two-runs / crashed-empty-leg-disclosed / basename-collision-refuses / framework-pin-desync-refuses / missing-corpus / missing-binary.

REAL DATA, BOTH DIRECTIONS, on the present build (scratch/bin/tomba2_port md5 4e21d13225122177bcf1945378197d2b, built 2026-08-12T16:45:49), one claim set (scratch/producers/claims.txt, 10 distinct claims over 126 lines): the two fresh legs run through tools/gate.py (scratch/k91c/warp9 = 'newgame / run 200 / warp 9 / run 300', scratch/k91c/hut = the hut-entry replay) read 10/10 RE-EARNED, exit 0, with both legs credited by RECORDED md5; the eight 15:23-15:33 legs in scratch/k91/*, which carry no binary identity and predate this binary, read 0 credited / 10 BUILD PROVENANCE UNKNOWN, exit 1 — neither certified nor called fossils. GATE PROVEN, not assumed: replacing `_ref_time_from_git`'s body with a 1970 constant turns --selftest RED (7 FAILs, naming the reference in each); stubbing the runs=-binding branches out turns it RED with 2 FAILs; restore reproduces md5 c0e208c9bb50a69a5c0ba658ce06f4f0 and 0 FAILs.

## Known failure modes

- **mtime is not a content identity.** A leg with no `binary.txt` cannot be credited at all, and even the recorded md5 is only as good as gate.py's before/after snapshot (a swap mid-run is recorded as MISMATCH and disqualified). The residual: a build made against a different `PSXPORT_DIR` produces a different md5 and is therefore correctly not credited, but nothing here can say WHICH tree it came from. The real fix is the `PSXPORT_BUILD_ID` (`git describe --always --dirty`) handover in kanban #91, after which each run carries its own build identity.
- **NOT-RE-EARNED is not DEAD, and the tool says so in every report.** A producer key is a function of guest state, so a claim goes un-earned whenever the corpus never visited the content that earns it (measured: 0x800803DC un-earned over 119 legs, earned on the first `warp 9`).
- **CORRECTION 2026-08-12 — an earlier version of this card recorded an incident that does NOT REPRODUCE.** It said the first version of the tool dated against HEAD and "wrongly called all 10 claims stale on a corpus in which 9 had just been earned". `git reflog` shows HEAD was 11a75fb for the entire measurement window (committed 15:01:02, superseded 15:53:39; the 15:37:03 entry is `reset: moving to HEAD`, a no-op), and 11a75fb's only change is the `external/psxport` pin — which IS in the key-deciding set. Both references therefore resolve to 11a75fb @ 2026-08-12T15:01:02 and produce byte-identical output on this tree, so that "measurement" cannot have happened as described. The HEAD-vs-key-path DISCRIMINATION is still correct and is now gated hermetically by the `docs-only-head` fixture instead of by anecdote.
- **CORRECTION 2026-08-12 — the "10/10 RE-EARNED over 7 legs" validation was computed over a corpus missing its most important member.** Eight legs were `cp`-ed into `scratch/k91/union`; seven files arrived. `hutalt` and `warp9` both finished at 15:30:49 and the runtime names run files at one-second resolution, so the copy collided and the survivor was hutalt's (md5 819a5ddc…; warp9's 6dfa400f… is absent from the union). warp9 is the leg kanban #91 and C043 call THE PROOF. The legs themselves were not lost — `scratch/k91/warp9/run-2026-08-12T15:30:49.jsonl` still exists — so the defect was the union, not the measurement corpus on disk. Fixed at cause: `--obs` is repeatable, legs are read in place, and a basename appearing in two leg dirs with identical bytes now REFUSES (exit 2) while differing bytes print a named warning. The validation above is the recomputation.
</content>
</invoke>
