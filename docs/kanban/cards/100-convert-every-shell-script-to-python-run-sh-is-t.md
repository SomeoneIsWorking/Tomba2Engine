---
id: 100
title: Convert every shell script to Python (run.sh is the only exception)
status: todo
labels: [debt,tooling,workflow]
created: 2026-08-16
updated: 2026-08-16
---

USER, 2026-08-16: "only run.sh should be a shell script, all other scripts should be python". Rule recorded in this repo's CLAUDE.md and in psxport's. run.sh is exempt because it is the user's launcher, not a tool.

INVENTORY (2026-08-20), 18 files:

Tomba2Engine/tools/ — 5
  ab_leg.sh  ab_replay.sh  beh_ab.sh  pan_gate.sh  warpsweep.sh

psxport/ — 13
  scripts/bootstrap-workspace.sh  scripts/build-openbios.sh  scripts/sync-submodules.sh
  tools/build_rmlui.sh  tools/build_xa_wavdump.sh  tools/clean.sh  tools/decomp.sh
  tools/fmv_compare/build.sh  tools/fmv_export/build.sh  tools/gen_gpu_shaders.sh
  tools/recomp/build.sh  tools/scratch_reset.sh  tools/syntaxcheck.sh  tools/test_gpu_render.sh

ORDER — by how often the thing is run and how much a silent failure costs:
  1. psxport/scripts/sync-submodules.sh — its denominator discipline is good and must survive the port.
  2. tools/pan_gate.sh, tools/ab_*.sh, tools/beh_ab.sh, tools/warpsweep.sh — A/B and sweep drivers.
  3. the build_*.sh / clean.sh / scratch_reset.sh helpers.
  4. decomp.sh — the Ghidra headless wrapper; careful, it is load-bearing for all RE work.

CONVERSION BAR (not a transliteration): argparse, a --help that states WHAT THE TOOL ASSERTS, and exit
codes that separate PASS / FAIL / REFUSED-because-it-could-not-assert-anything. Every negative result
prints its denominator. tools/present_gate.py is the reference shape.

Bash word-splitting bit this on day one: the first draft of present_gate.sh split its scene table on
spaces and 'skipped' 29 nonexistent replays whose names were fragments of the description column, while
still exiting green on the legs it did run. That class of bug does not exist in the Python version.
