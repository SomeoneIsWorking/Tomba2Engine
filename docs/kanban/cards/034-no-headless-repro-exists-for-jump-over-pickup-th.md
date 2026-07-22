---
id: 34
title: No headless repro exists for jump-over-pickup — the #1/#30 fix is RE-proven but symptom-unverified
status: todo
labels: [verification]
created: 2026-07-22
updated: 2026-07-22
---

The #1/#30 fix (zero-extending the two accept gates in ActorTomba::proximityCheck, commit 5c7b1bd) is proven from the guest instruction stream — the guest's andi zero-extends, the sibling subHitboxCheck already does, and the measured accept window (-280..+130 against a ~370-unit jump) explains the symptom exactly. It is also proven INERT where the defect never fires: 2 MB RAM dump byte-identical pre/post on the bucket replay, and genuine contact still collects. What is NOT established is the symptom itself: nobody has watched a jump-over fail to collect, before or after, because no headless route was found that gets Tomba jumping over a ground item in the aux list. Every item the seaside routes reach is either 800+ units above him or snaps to his own position before being tested. BUILD THE REPRO: teleport is the obvious lever — ActorTomba node is 0x800E7E80 with world position as s16 at +0x2E/+0x32/+0x36, writable from the REPL with  22:08:13 up 3 days,  3:16,  1 user,  load average: 3.55, 5.23, 6.55
USER     TTY       LOGIN@   IDLE   JCPU   PCPU  WHAT
user   tty1      Sun18    3days  0.09s  0.09s startplasma-wayland, so place Tomba just short of a known ground item, jump, and watch the item's state byte go to 2. Record it as replays/bugs/jump-over-item.pad (the agent left a placeholder file of that name). Until that exists, treat #1/#30 as RE-correct but symptom-unverified, and if the USER still sees the pickup happen, this repro is the first thing to build rather than re-reading the gates.
