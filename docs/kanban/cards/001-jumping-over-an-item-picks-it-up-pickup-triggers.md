---
id: 1
title: Jumping over an item picks it up — pickup triggers without touch contact
status: done
labels: [bug, pc-skip]
created: 2026-07-17
updated: 2026-07-22
---

USER 2026-07-16: jumping over an item picks it up; normally you have to touch the item. Suspect the pickup trigger's collision volume/overlap test (beh_pickup_collect_trigger family) — vertical extent too tall or Y ignored in the overlap test. May be a faithful-port divergence or genuine stock behavior misremembered — verify against the recomp_path oracle first. Investigate when the pickup/collision RE is on the frontier.

**2026-07-22:** ✅ USER CONFIRMED FIXED (2026-07-22), same fix as #8: the signed-16-bit read of the ride/attach pointer G+0x158 in Behaviors::areaSeasidePerframe (mem_r16s instead of the guest's 32-bit unsigned lw+sltiu at 0x80113CC0). Attached/holding, (int16)0x800FB960 = -18080 < 2 is TRUE, so the mode-2 sub-tick FUN_8011334C fired in exactly the states the guest suppresses it - which is a coherent mechanism for a pickup triggering without touch contact. Unattached both readings agree, so the defect only ever showed up mid-interaction. See docs/findings/ai.md 'the defect'.
