---
id: 121
title: Default launcher aborts in title attract when adjacent OT keys collapse to one float order
status: todo
labels: [bug, render]
created: 2026-08-21
updated: 2026-08-21
---

Discovered during the 2026-08-21 no-argument launcher sanity after the bounded water-jet fallback. Reproduces twice at native_boot attract frame about 1950: RenderQueue::resolveKeyOrderFaces aborts because key 1920 and its nearer band both map to float order 0.011602190, violating the strict-monotone invariant. Immediate cause is adjacent OT-key order values collapsing to the same float; proper fix belongs in the authoritative OT-key-to-order mapping, not by weakening/skipping the verifier. Attribution run with PSXPORT_DEBUG=gtefallback shows the last water-jet fallback call at f1164 and the abort at f1950, so it is not a same-frame fallback submission. Evidence: scratch/logs/waterjet_default_launcher.log and waterjet_default_launcher_attribution.log.
