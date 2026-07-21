---
id: 7
title: DEMO OPTIONS sub-pages (Messages/Sound/Screen adjust/Controls) had no pc_render producer — SIGABRT two presses from the title
status: done
labels: [render]
created: 2026-07-21
updated: 2026-07-21
---

Reached: title -> Right -> Cross (Options) -> Cross. leaf_8007B45C writes sm[0x50]=cursor+1 on the first Cross, selecting one of the 5 page builders (FUN_8007F104/F250/F498/F73C/F8F8); only page0 had a producer, so Render::optionsPageNative aborted. FIXED: one producer per page in game/render/render_options.cpp (Render::optionsSelectPage/MessagesPage/SoundPage/ScreenAdjustPage/ControlsPage) + Render::emitMenuSprites (FUN_8007E6DC sprite-template decoder) for the Controls pad diagram, + Render::optionsSolidBox (FUN_8007FCC8) for the Screen-adjust value boxes. All four pages screenshot-verified against the PSXPORT_RENDER_PSX=1 reference.
