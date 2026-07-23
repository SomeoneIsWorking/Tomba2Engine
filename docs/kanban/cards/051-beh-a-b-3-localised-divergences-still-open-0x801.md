---
id: 51
title: beh_* A/B: 3 localised divergences still open (0x8013C9C0, 0x8011D988, 0x80121978)
status: todo
labels: [verification, bug]
created: 2026-07-23
updated: 2026-07-23
---

Split from #10's sweep 2026-07-23. The A/B rig (tools/beh_ab.sh, PSXPORT_BEH_SUBSTRATE + 2MB RAM + .spad diff) got 52 of 54 reached handlers to zero-diff on bucket-softlock. These three remain localised but unexplained — each is a REAL divergence between the native rebuild and the guest body, i.e. the same confirmed bug class that produced #2, #1/#30 and the ZonedAttacker arm-shift.
  - 0x8013C9C0 (bucket, 100 B): the native writes node[1]=1 at f160 that the guest body never writes. No obj+1 store exists in that file, so it comes from a CALLEE the native reaches and gen does not — the dispatch forks (native -> leaf_80024F18 / fieldTargetCursor / advanceLinkChain; gen -> two Math::isqrt16). Chase the dispatch fork, not the file.
  - 0x8011D988 (house-on-the-point, 2071 B) and 0x80121978 (house-on-the-point, 30 B): BOTH first diverge at the graphics-record freelist cursor 0x800E7E74 + count 0x800ED098 — a different NUMBER of records bound. For 0x8011D988 all 543 invocations' node deltas and the whole registry dispatch stream are identical after the cull fix, so the cause is an inline eng(c).* native or a callee's arguments.
    ** NOTE THE OVERLAP: house-on-the-point is kanban #47's repro. The agent flagged these may be #47's defect seen from the other end — the transition-enter truncation (#50) leaves state unestablished, which could plausibly change how many graphics records get bound. CHECK #50/#47 FIRST before hunting these two independently; they may vanish when the systemic yield-truncation is fixed.
Rig + instruments to reuse: tools/beh_ab.sh (cov/base/one/sweep), PSXPORT_DEBUG=behcov (per-handler reach counter), PSXPORT_BEH_TRACE=<addr> + behtrace (per-invocation node-byte delta logged at the dispatch site so both legs' logs are line-aligned and diff names the first divergent invocation/object/field).
