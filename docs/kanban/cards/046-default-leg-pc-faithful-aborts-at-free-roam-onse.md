---
id: 46
title: recomp-MISS 0x80028E64 latent (label not emitted as entry) — NOT reproduced on current main
status: todo
labels: [bug, recomp]
created: 2026-07-23
updated: 2026-07-23
---

Flagged by the fx_sprite port agent 2026-07-23 (NOT caused by it — pre-existing, and it verified via GATE=1 instead). The default pc_faithful leg aborts entering free-roam with recomp-MISS 0x80028E64. Same class as kanban #24/#27: a recompiler function-boundary discovery gap — 0x80028E64 is a mid-body label of gen_func_80028E10 that gen_func_8003116C (intro-narration effect family) jumps to, and the recompiler never emitted it as an entry. Fix is the same shape as #24: seed the mid-body entry / area-indexed-table discovery in external/psxport/tools/recomp/emit.py and regenerate, OR port the function. Note this blocks default-leg verification of anything past free-roam onset in the current checkout — the fx_sprite flame port was gated on GATE=1 + psx_render for exactly this reason, so once this is fixed, re-verify the flame on the default leg too.

**2026-07-23:** 2026-07-23 SYMPTOM NOT REPRODUCIBLE on current main — 'aborts at free-roam onset' is FALSIFIED. Verified clean (rc=0, zero recomp-MISS) on: default leg newgame->intro-narration->run 6000; default leg AUTO_SKIP reaching free-roam at frame 216 and rendering the seaside field correctly (scratch/screenshots/def_freeroam.png — this is the ./run.sh pc_faithful+pc_render config); default leg full seesaw-weight.pad replay through 16500 frames incl. the whole intro; and GATE leg newgame run 6000. The fx_sprite port agent hit this in its OWN WORKTREE, which branched from a STALE squashed base (f646cb5) predating the recompiler fixes #24/#27; that stale checkout state produced the miss, not current main. THE UNDERLYING GAP IS STILL REAL BUT LATENT/UNREACHED: 0x80028E64 is a case-label inside gen_func_80028E10's internal jump-table switch (generated/shard_7.c:2075), reachable only via that function's own switch on c->r[2] — there is no standalone func_80028E64 entry, so IF some caller ever does a direct func_80028E64(c)/rec_dispatch(c,0x80028E64) it will miss. None of the tested boot->free-roam->seaside paths reach that, so this is downgraded from a live abort to a latent discovery gap; re-open with a concrete repro if it recurs. Not blocking — the default leg boots to free-roam and renders.
