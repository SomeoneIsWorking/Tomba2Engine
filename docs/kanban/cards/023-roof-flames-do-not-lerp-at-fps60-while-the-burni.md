---
id: 23
title: Roof flames do not lerp at fps60 while the burning-rope flame does
status: todo
labels: [render,fps60]
created: 2026-07-22
updated: 2026-07-22
evidence: docs/reference/issues/issue23_flame_no_lerp.png
---

USER 2026-07-22 with a matched pair, which is what makes this diagnosable: docs/reference/issues/issue23_flame_lerps_ok.png is a flame that DOES interpolate correctly (the dim flame burning along the rope/vine), and docs/reference/issues/issue23_flame_no_lerp.png is one that does NOT (the two bright flames on the hut roof). Two flame producers, only one of them interpolating. THE MECHANISM IS ALREADY DOCUMENTED - do not re-derive it. The fps60 audit in docs/findings/render.md states the honest remainder as: 'RQ_WORLD items with has_xyf==0 (guest-time records without a display-pass producer yet) and screen-space HUD/2D present VERBATIM on both frame kinds - they step at 30 Hz'. A layer that renders but presents verbatim is exactly a layer that does not lerp. PRIME HYPOTHESIS, CHECK IT FIRST: the roof flames are very likely the ones restored by the #12 fix, i.e. the FUN_80027A4C sprite-family tap in game/render/fx_sprite.cpp. That tap runs at GUEST time on the real frame and re-derives its quads there; if it is not also driven from the DISPLAY PASS (the interp re-run under lerped camera, tomba_fps60_world_pass / Fps60::tier1Render), its output can only present verbatim - the layer comes back but never interpolates. That would mean #12 restored the picture without finishing the job, which is a fair description of a partially-landed producer rather than a new bug. Confirm by identifying which producer draws each flame (debug objid labels what fieldObjectsRender renders; the fx tap is separate) before fixing. THE FIX IS THE LERP CLAUSE IN CLAUDE.md: give the producer a display-pass path so it re-derives under the lerped camera like every other native producer. NOT a matcher, NOT an anchor/stamp special case - both explicitly banned.
