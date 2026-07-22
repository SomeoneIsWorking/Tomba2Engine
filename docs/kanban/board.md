# Kanban board

> The project bug/issue tracker. Managed by `tools/kanban.py` (see skill `bug-tracker`).
> Cards live in `docs/kanban/cards/`; evidence images in `docs/reference/issues/`.

## BACKLOG (0)

## TODO (16)
- **#9 pc_skip should skip the LOADING SCREEN entirely (not just its text)**  `pc-skip,enhancement`
- **#10 Verify the ~58 untested beh_* rebuilds by faithful-body A/B (finds bugs of the confirmed class)**  `verification`
- **#14 Weapon CHARGE effect missing under pc_render (hold attack)**  `render` — 📎 docs/reference/issues/issue14_charge_swing.png
- **#15 Weapon IMPACT effect missing under pc_render (hitting something)**  `render`
- **#16 Sign text jitters at fps60 when the camera moves**  `render,fps60` — 📎 docs/reference/issues/issue16_sign_text_jitter.png
- **#18 Score/AP-gem pickup display effect missing under pc_render**  `render`
- **#22 Health wheel is too transparent — reproduces on psx_render too, so the oracle is NOT the reference**  `render` — 📎 docs/reference/issues/issue22_health_wheel_reference.png,    docs/reference/issues/issue22_health_wheel_reference_dark.png
- **#23 Roof flames do not lerp at fps60 while the burning-rope flame does**  `render,fps60` — 📎 docs/reference/issues/issue23_flame_no_lerp.png
- **#26 Temple interior (area 12): ceiling beam band renders warped/displaced under pc_render**  `render`
- **#31 fps60: interpolated frames and real frames appear to be built differently**  `bug,render,fps60`
- **#32 17 guest addresses have two native owners — the class that caused #28**  `workflow`
- **#33 fps60 unification made the world render twice per logic frame — remove the now-dead guest-time world build**  `render,fps60,perf`
- **#34 No headless repro exists for jump-over-pickup — the #1/#30 fix is RE-proven but symptom-unverified**  `verification`
- **#35 START pause page (Options / Load data / Quit game) draws with no panel background**  ``
- **#36 dev-warp: cold cross-area warp self-destructs ~50 frames later, and ids >=22 are accepted but are not areas**  `bug,tooling`
- **#37 areas 16/17/18 hang under cold warp — behaviour loop in gen_func_80040558**  `bug`

## DOING (1)
- **#2 Bucket-pickup cutscene SOFTLOCKS with pc_skip ON (default config)**  `bug,pc-skip`

## DONE (20)
- **#1 Jumping over an item picks it up — pickup triggers without touch contact**  `bug,pc-skip`
- **#3 RmlUi not working**  `ui`
- **#4 Widescreen-from-boot corrupts objects (flower/gem, attack weapon)**  `render`
- **#5 Save-sign inspect SOFTLOCKS with pc_skip ON**  `bug,pc-skip`
- **#6 Load-Game browser (DEMO s48==4) aborts: rec_dispatch miss 0x8018FA88**  `bug,recomp`
- **#7 DEMO OPTIONS sub-pages (Messages/Sound/Screen adjust/Controls) had no pc_render producer — SIGABRT two presses from the title**  `render`
- **#8 Water-pump seesaw: Tomba's weight doesn't pull it down when grabbed while climbing (pc_skip ON)**  `bug,pc-skip`
- **#11 Barrel top face renders BLACK on the blue side (red side correct)**  `bug,render`
- **#12 Torch flame effect missing entirely under pc_render**  `render` — 📎 docs/reference/issues/issue12_13_missing_flame_object.png
- **#13 HUD weapon carousel missing entirely under pc_render**  `render` — 📎 docs/reference/issues/issue12_13_missing_flame_object.png
- **#17 Barrel top surface FLICKERS at fps60 (regression from the #11 sort-key fix)**  `render,fps60` — 📎 docs/reference/issues/issue12_13_pc_vs_oracle.png
- **#19 Dialog text-box background missing — only the panel corners render**  `render` — 📎 docs/reference/issues/issue19_panel_fill_missing.png
- **#20 Pause (P) shows a black screen at fps60 — the pause loop RE-RENDERED the frame instead of re-showing it**  `render,fps60`
- **#21 Triangle menu renders fully transparent — its opaque background is missing**  `render` — 📎 docs/reference/issues/issue21_triangle_menu_reference.png
- **#24 Area 22 aborts on entry: rec_dispatch miss 0x80109200**  `bug,recomp`
- **#25 pc_render area sweep: 24 of 32 areas sampled clean, coverage recorded**  `render`
- **#27 recomp: misread jump-table base blocked four areas (10/11/13/14) — FIXED**  `recomp`
- **#28 Dialog box background draws OVER its own text — dual ownership of 0x8004FFB4**  `bug,render`
- **#29 Hut interior: wall decorations z-fight with the wall behind them**  `bug,render`
- **#30 Jumping over an item picks it up again — REGRESSION of #1**  `bug,pc-skip`
