---
id: 107
title: renderpath toggle shifts the picture centre when widescreen is on
status: todo
labels: [render, bug]
created: 2026-08-19
updated: 2026-08-19
---

USER 2026-08-19, live windowed run, machinery cutscene: toggling native<->psx with WIDESCREEN ENABLED shifts the centre point of the picture. Deferred by the user behind the missing-graphics work — noted, not yet fixed.

A USEFUL SIDE EFFECT, and a confirmed datum: the shift is what revealed the BRIDGE ROPES. They render fine on the psx path; on the native path they simply fell outside the 4:3 view. So #103's 'bridge ropes missing' is NOT a producer gap — it is this centring bug plus a crop. USER: 'PSX render also shows the ropes now ... which is kind of a good thing since it verifies that the ropes should be visible here but ofc the shifting needs to be fixed'.

SUSPECTED MECHANISM, read from the code, NOT measured — do not treat as root cause:
  gpu_vk.cpp video_inputs() gates aspect on rsub.mode.enhancementsAllowed(), so switching to psx forces ASPECT_4_3 and gpu_vk_wide_engine(c) goes false. GpuVkState::present then stops widening disp_w to gpu_vk_wide_engine_w() and presents the 320 column span instead of the ~428 wide one. present_plan.h computes the viewport from disp_w (pane_letterbox(4*disp_w, 3*native_w, ...)), so the presented rect changes shape between the two paths.
  That much is BY DESIGN (a pure path must not be widened). What is NOT explained is the CENTRE moving: the wide engine renders into VRAM columns [sx, sx+nw) with gpu_vk_wide_engine_ofx = nw/2, and the sx the psx blit uses may not be the same origin the wide native pass was composed around. Measure sx / s_present_sx / s_last_w on both paths at a FIXED frame before believing any of this.

Falsifier: if sx is identical on both paths at the same frame, the origin theory is wrong and the shift lives in the viewport rect alone.
