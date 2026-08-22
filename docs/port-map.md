# RE + port frontier — is each step REAL (re-verified) or a HACK? (managed by tools/portmap.py)

The RE dependency chain. `## ` block per step. Work `portmap.py next`; kill `portmap.py hacks`.
Detail lives in docs/port-progress.md; this is the queryable real-vs-hack frontier.

**Status:** 32 verified · 14 ported-unverified · 2 hack · 3 todo · 3 blocked

## title-frontend — DEMO stage s0..s7 + menu logic
- **scope:** 0x801062E4 stage; Demo::s0..s7; sub-machines 0x8010696C/0x80106AC4
- **status:** verified
- **order:** 10
- **owner:** game/scene/demo.cpp
- **notes:** s2SubMachine owned this session (last unowned title sub-machine); SBS 0-diff (see parity.py)

## title-render — logo+menu+cursor geometry
- **scope:** titleNative + shared data-driven menu emitter (menuChrome/menuItemsAndCursor/emitMenuFt4)
- **status:** verified
- **order:** 11
- **deps:** title-frontend — DEMO stage s0..s7 + menu logic
- **owner:** game/render/render_walk.cpp
- **notes:** now DATA-DRIVEN (reads guest FT4 templates via FUN_8007e1b8 reproduction) — hand-decoded constants retired; title pc-vs-psx RMSE 0.000 (no regression). Shared with s3MenuNative (#2b).

## newgame-sop-intro (#2b)
- **scope:** DEMO sm[0x48]==3 = the s3 MENU page (logos + FT4 items 0x90/0x91 + cursor). NOT SOP narration.
- **status:** verified
- **order:** 20
- **deps:** title-render — logo+menu+cursor geometry
- **owner:** game/render/render_walk.cpp (s3MenuNative + shared menuChrome/menuItemsAndCursor/emitMenuFt4)
- **notes:** BUILT + VERIFIED: data-driven menu emitter reproduces FUN_8007e1b8/FUN_80106824 reading guest templates; s3 menu pc-vs-psx RMSE 0.000, no crash at s48=3. Draw order: items then cursor (cursor on top in overlap).

## collision-resolve-23d48
- **scope:** FUN_80023D48 — actor-vs-object cylinder collision resolve + push-out
- **status:** ported-unverified
- **order:** 20
- **owner:** game/world/collision_resolve.cpp CollisionResolve::cylinderResolve
- **notes:** PORTED + SBS-gated 2026-07-29; READABILITY PASS COMPLETE 2026-07-30. Both bodies in this file (0x80023D48 cylinderResolve, 0x8002423C landOnObjectTop) now read as named control flow over named values: L_8002xxxx gotos replaced by real if/else + early returns, register chains lifted into named locals or GuestReg<N> proxies, the frame is a GuestFrame<80,10>/GuestFrame<32,3> from the abi_extract --scaffold --guestabi contract, and the eight call sites are guest_call(c, kRa..., func_XXXX). The three per-record field tables (actor r17 / other r22 / anchor r30) stay SEPARATE - actor+0x30 is a 16.16 Y, anchor+0x30 is the anchor's Y. Re-gated 2026-07-30: port_check PASS on both methods; recorded 1500-frame SBS gate re-run gives 50/50 A/B-identical, ovhit 0x80023D48 7397/7397 and 0x8002423C 1949/1949. The stack hazard stays HANDLED: callees go through generated func_XXXX wrappers, never the Trig methods (callee audit in docs/re/collision-resolve-23d48.md proves rsin's own-frame ra word is the only guest-stack byte at stake). Decision structure, sign convention and the +0x80/+0x82 two-radii observation are documented in the same file.

## sop-narration-void-vortex (#5)
- **scope:** SOP void beat (0x800BF9B4==5): vortex object 0x800FBA68 not rendering under pc_render
- **status:** verified
- **order:** 22
- **deps:** newgame->field transition (s5 leave-demo)
- **owner:** game/render/narration_swirl.cpp (Render::narrationSwirlRender)
- **notes:** FIXED: the swirl is a type-0x20 CUSTOM-RENDER-FN node (0x8010BF54, SOP overlay) the native walk skipped. Full RE'd native producer (mesh 0x8010CC08 36B quad records, rotmat*rotY*colScale transform via projComposeObjectHost, 2 blades, U-scroll anim). Verified: coverage 59.1% vs ref 58.9%, RMSE 20 (accepted-3D band; field=61), read-only (DisplayPassGuard silent 600+ frames), title RMSE 0 + hut replay regressions clean. USER eyeball for animated result pending.

## newgame->field transition (s5 leave-demo)
- **scope:** DEMO sm[0x48]==5 (demo_frame_s5, LEAVE-DEMO teardown) + GAME s48=5 stale-handoff
- **status:** verified
- **order:** 25
- **deps:** newgame-sop-intro (#2b)
- **owner:** game/render/render_walk.cpp (renderTitle s48==5 -> renderLoading)
- **notes:** s5 = ~2-frame task teardown (jal 0x80052078(2)), OT empty -> black on the reference. Routed to black loading; s4 load-browser/s6/s7 still crash (real content). VERIFIED: pc_render now boots title->New Game->walkable field (GAME 0x8010637C) with NO crash, stable 300+ field frames; title unchanged.

## cinematic-letterbox-bars
- **scope:** UI-effect manager 0x80100400 slot type 1 (FUN_80026864) — cutscene top/bottom black bars
- **status:** verified
- **order:** 26
- **deps:** field-world (sceneNative)
- **owner:** game/render/cine_bars.cpp (Render::cineBarsRender)
- **notes:** CUSTOM PC-NATIVE (USER: don't transcribe PSX, make it wide/60-adjustable). Reads guest slot only as signal (active + progress 0..1); bars are a native overlay sized to the DISPLAY: full-width overdrew for any aspect (wide margins covered), symmetric flush bars, re-emitted every present (progress is the one live knob an fps60 tier can lerp). Not oracle-matched (oracle letterbox is itself buggy). Screenshot sent.

## field-world (sceneNative)
- **scope:** 0x8010637C GAME field: terrain+entities+objects+backdrop, real depth
- **status:** ported-unverified
- **order:** 30
- **deps:** title-frontend — DEMO stage s0..s7 + menu logic
- **owner:** game/render/render_walk.cpp (sceneNative)
- **notes:** renders; not SBS-gated this session — add a parity entry when driven under SBS

## field-2D layer (#3b)
- **scope:** field HUD/dialog/billboards/op-0x7C sprites — the free-roam blocker
- **status:** ported-unverified
- **order:** 31
- **deps:** field-world (sceneNative)
- **owner:** -
- **notes:** Track B LANDED: font->queue + panel taps + dialogTextNative + gauge text-row tap (FUN_8004EB94, parity=partial — needs gauge-popping drive). Track A LANDED: tile-grid layer owned (TileGridLayer, parity=verified f20820); backdropRender owns its picture. Remaining: gauge firing drive + USER eyeball of the whole 2D layer; special-char icon glyphs (FUN_80078988) still substrate.

## render-billboard-c788
- **scope:** render handler 0x8003C788
- **status:** ported-unverified
- **order:** 40
- **owner:** perobj_billboard.cpp::billboardComposeC788
- **notes:** Render::billboardCompose3 (perobj_billboard.cpp): identity(MAT_A)+matMul(node+152)→shared CAM2 tail on MAT_ROTZ→billboardEmit; owned helpers Mtx::identity/Math::matMul; build-clean + abi audit OK (frame32/spills/ra); byte-faithful by construction like SBS-gated C2D4/C464. Needs SBS 0-diff gate when a disc is available.

## render-mat-847f0
- **scope:** math leaf 0x800847F0
- **status:** verified
- **order:** 41
- **owner:** game/math/gte_math.cpp::Math::rotMatSoft
- **notes:** SW (non-GTE) 3-Euler RotMatrix; owned via overrides::install(0x800847F0). SBS-full 0-diff f0..f360 (billboard C5F8 fed it 8x, MAT_ROTZ byte-identical).

## render-overlay-submitblock-146478
- **scope:** FUN_80146478 — A00 field submit-block dispatcher (splits the packed GT3/GT4 count header, chains the two leaves through v0)
- **status:** verified
- **order:** 41
- **owner:** game/render/overlay_gt3gt4.cpp OverlayGt3Gt4::submitBlock
- **notes:** Was the busiest remaining rec_dispatch target in the game: 127,275 hits per 6000 frames of replays/bugs/seesaw-weight.pad, 4x the runner-up. Frame contract from abi_extract.py --contract (32-byte frame, s1/ra/s0 at +20/+24/+16, ra constants 0x8014649C/0x801464AC) rather than hand-derived. SBS 0-diff f1500 with ovhit proving 76378 executions on EACH leg. Leaf calls deliberately routed through the generated ov_a00_func_* wrappers so the registry hit counters stay truthful — a direct call works but makes both leaves report NEVER HIT.

## render-billboard-c5f8
- **scope:** render handler 0x8003C5F8
- **status:** verified
- **order:** 42
- **deps:** render-mat-847f0
- **owner:** game/render/perobj_billboard.cpp::Render::billboardComposeC5F8
- **notes:** 4th compose sibling = C2D4 with rotZ->rotMatSoft(node+84). SBS-full 0-diff f0..f360, ovhit C5F8 native=8/oracle=8 (equal).

## render-screenfade-726d4
- **scope:** render handler 0x800726D4
- **status:** ported-unverified
- **order:** 43
- **owner:** game/render/screen_fade.cpp::Render::fadeTileRender
- **notes:** Native producer for the full-screen fade/flash tile (guest FUN_800726D4, render-walk case 0x8003C138 = node case-byte node[+0x0b]==8). Build-clean, read-only. STILL RUNTIME-UNVERIFIED, now with the reason understood: the game has TWO unrelated fade paths and this is the RARE one — ScreenFade (leaf tap FUN_8007E9C8, already native) is what runs in essentially every scene. Decoded the 33-entry walk table @0x80014DB8; PSXPORT_DEBUG=walk shows only 3 targets ever fire across the whole replay library (C29C/C0B4/C0E8), so idx 8 is cold, not broken. See docs/findings/render.md 'Two distinct fade mechanisms'. Next: find the constructor writing 8 to node[+0x0b].

## render-effectmod
- **scope:** secondary-effect handlers 0x8003F3F4/F4C4/F344/F594/D584
- **status:** verified
- **order:** 44
- **owner:** perobj_dispatch EffectMod latch + submit.cpp
- **notes:** PORTED + VERIFIED. game/render/effect_mod.cpp — five Render methods (effectSemiOn/SemiOff/ClutSwap/FlatTint/ColorAdd) replacing the substrate leaves FUN_8003F3F4/F4C4/F344/F594/D584; wired at the perobj_billboard CCA4 call sites. Written with typed lenses (GpuPacket, EffectParams, PacketShape) instead of raw mem_rXX(p+0xNN) soup. VERIFIED by a differential oracle test (PSXPORT_SELFTEST=effectmod, game/render/effect_mod_selftest.cpp): synthetic packet pools swept across every opcode + all three coloradd regimes, native vs rec_interp of the real MAIN.EXE, 2000 runs, 0 mismatching words, 0 oracle-skipped. Gate proven meaningful by mutation testing — a 1-bit change to the 0x7F bias and the Ghidra cmd-byte ordering bug are both caught. Unblocks render-mesh-flush.

## fx-sprite-writer-328ec
- **status:** ported-unverified
- **order:** 44
- **owner:** game/render/fx_sprite.cpp
- **notes:** CORRECTED 2026-07-28: only ONE of the three producers is verified, so this step is ported-unverified as a whole. The earlier note read 'VERIFIED: 228 emissions on walk-dust-puff and seesaw-weight' — every one of those 228 (456 across both replays) carries gate=-64, i.e. they are ALL Render::waterJetSpriteRender. Render::fxAltAnimSpriteRender (0x8012E868) and Render::fxRotSpriteTailRender (0x8012D9E8) are COLD across the entire 15-replay library: zero emissions, checked explicitly by their distinguishing gate/depth signature, not assumed.

## render-mesh-flush
- **scope:** mesh-flush 0x8003F174/0x8003EF9C
- **status:** blocked
- **order:** 45
- **deps:** render-effectmod
- **owner:** submit.cpp shared per-cmd flush
- **notes:** PARTIAL: own GENERIC-mode loop only; overlay-mode geomblks (0x8012/0x8013xxxx) are the SEAM — next tier, do NOT jump

## render-hut-interior
- **status:** verified
- **order:** 46
- **owner:** game/render/render_hut_interior.cpp::Render::renderHutInterior
- **notes:** Reduced OBJECTS-ONLY producer (fieldObjectsRender: room 0x800FD850 + NPCs + Tomba, real depth, live interior camera). Was abortUnimplemented. VERIFIED: pc_render f410 shows room+NPCs+Tomba+props, no village leak, no crash/guest-write. fps60 flicker-gated in fps60_worldpass.cpp.

## render-title-substates
- **status:** verified
- **order:** 47
- **owner:** game/render/{card_browser,render_options,render_attract}.cpp
- **notes:** renderCardBrowser(s48==4) VERIFIED: reached headless (tap x at title), renders correctly (scratch/screenshots/card_browser.png). renderAttract(s48==7) VERIFIED: idle ~1100 frames at title auto-enters attract, full 3D field render (substate_s7.png). OPTIONS (s48==6) VERIFIED 2026-07-23 and the old 'not reachable by title nav' note is FALSIFIED — the route is title -> Cross (New Game menu, s48==3) -> Right (Options) -> Cross, captured as replays/bugs/title-options-page.pad (page 0 at frame 1027). All five pages are 0/76800 vs psx_render there; the page itself is produced by OptionsPage (see render-options-subpages), so renderTitle's s48==6 branch now only supplies the title chrome under the Screen-adjust page.

## render-options-subpages
- **status:** verified
- **order:** 48
- **owner:** game/ui/options_page.cpp (class OptionsPage) + game/render/render_options.cpp
- **notes:** kanban #7 then #38. ONE producer for BOTH entry points (title front-end sm[0x48]==6 and the in-game dispatcher FUN_8010810C page byte task-sm[0x6B]==3, which share the five builders FUN_8007F104/F250/F498/F73C/F8F8). Each element is produced at ITS OWN guest emitter under a page scope, not as a host twin of the page's element list: FUN_8007FC24 PORTED (OptionsPage::pushBackdrop, port_check PASS) and drawn at RQ_OVERLAY in the 2D-FG band; FUN_8007FCC8's boxes recorded from their existing single owner Panel::pushDialogBackdrop; cursor + pad diagram captured off the shared 2D group leaves via UiGroupCapture. render_options.cpp keeps only the two draw helpers + the title chrome Demo::s6 composites under the Screen-adjust page. GATE (pc_render vs psx_render, same frame, render_cmp.py): in-game Select Options 74442/76800 -> 0/76800, in-game Messages/Sound/Controls 0/76800, title-path all five pages 0/76800; psx leg unchanged vs the pre-port build (0/76800), so the ported packet is guest-equivalent. Repros: replays/bugs/ingame-options-page.pad f1160, title-options-page.pad f1027.

## render-producer-effect-mesh-family
- **scope:** the EFFECT-MESH family: shared writer FUN_80027768 and its 20 controller call sites — impact plume, weapon swing/charge, water jet mesh, and 14 further controllers
- **status:** todo
- **order:** 50
- **owner:** -
- **absent:** the effect-mesh PICTURE was deleted 2026-08-04 with the GTE-register taps (commit abf3cf9 removed game/render/fx_mesh.cpp/.h, mesh_emit_tap.cpp, swing_fx.cpp/.h). Those producers re-derived quads host-side from the transform the substrate controller had just composed into GTE CR0-7. Deleting them was CORRECT. Three controller-state replacements are live; seventeen native controller producers remain absent. One of those pictures, the `0x8013D454` water-jet mesh, is visible through the explicit `render-fallback-water-jet-guest-gte` hack; sixteen producer-less pictures still have no route. Do not widen that fallback to get the rest back.
- **notes:** OPENED 2026-08-06 by the G10 unported-render survey. UNRECORDED CONSEQUENCE of the tap retirement: mesh_emit_tap.cpp was the SINGLE owner of FUN_80027768 and dispatched to whichever controller SCOPE was up. Three of the 20 controller pictures now have display-pass owners: FUN_8002BC9C (four-copy plume), FUN_8002A834 (weapon-charge starburst), and FUN_800288AC (weapon-impact plume). Seventeen controllers remain without native producers; tools/codemap.py --addr now maps 0x80027768 only to the scoped water-jet hack and its frontier warning, while the A00 overlay three 0x8013D828/0x8013ED08/0x8013EF58 still have no owner. `0x8013D454`'s non-zero-mode mesh branch is rendered from exact guest GTE packets at logic time under explicit user authorisation, but remains in this row until it has a node-state display producer. Claim C011 remains falsified. REAL FIX: one native producer per controller, reading the controller's OWN node state and projecting with the native camera — the shape fx_sprite.cpp/fx_dust.cpp/fx_line.cpp already use. The RE is largely DONE and must not be re-derived: kanban #15 carries the impact/plume decode, and docs/findings/render.md 'The A00-overlay effect-mesh controllers' carries 0x8013D454/D828/ED08/EF58. Death condition: codemap --addr finds an owner for every controller in the 20-caller census in docs/findings/render.md.

## render-producer-plume-bc9c
- **scope:** FUN_8002BC9C — the FOUR-COPY RADIAL PLUME, the most resident of the 20 effect-mesh controllers left picture-less by the 2026-08-04 tap retirement (unported-render-inventory R1)
- **status:** ported-unverified
- **order:** 51
- **owner:** Render::radialPlumeRender (game/render/fx_plume.cpp)
- **notes:** PORTED 2026-08-06. RE from ground truth generated/shard_0.c gen_func_8002BC9C (controller) + generated/shard_5.c gen_func_80027768 (the shared 36-byte-record mesh writer, whose format Render::meshQuadRecordsEmit already owned). WHAT THE CONTROLLER DOES: publishes depth-cue IR0=0 (scratchpad 0x1F800090) and far colour CR21-23=0, i.e. programs the writer's DPCT/DPCS cue to the IDENTITY so the mesh keeps its authored colours; reads the animation-script byte at *(node+0x3C), whose low 7 bits index the node's own mesh-pointer table at *(node+0x50) and whose bit 7 marks the script's last frame; then FOUR times builds rotmat(node+0x48/4A/4C), column-scales it by the authored triple at 0x800A1CD4 (each byte << 2), composes with the scene camera, translates by the node's own s16 world position at node+0x2C/2E/30 and calls the writer with sortBias=(s16)node+0x32 and no U scroll, advancing the Y angle by a quarter turn between copies. NOT A TAP: the deleted fx_mesh.cpp producer took its transform from gte_read_ctrl(0..7); this one takes three node angle fields, one node position and one authored scale triple, and projects with the native fps60-lerped camera through projComposeObjectHost — so it is a DISPLAY-PASS producer that interpolates, dispatched from fieldObjectsRender's type-0x20 walk on node+0x18 == 0x8002BC9C (MAIN.EXE, so no overlay-residency signature is needed). THE Y ANGLE IS SAFE TO READ AFTER THE GUEST MUTATED IT: the guest leaves node+0x4A at base + a full turn and the engine sine LUT is a full 4096-entry turn, so display time names the same four orientations. NEW SHARED MECHANISM: MeshOtBias (mesh_quads.h) carries the writer's own per-quad ordering decision — AVSZ4 average + the caller's sort bias, compressed to a bucket and rejected outside [4,2048). ZSF4 is the game's OWN authored constant 256 (gen_func_80083FF8 sets ZSF3=341 ZSF4=256 H=1000 DQA=-4194 DQB=320<<16), so the key is mean-depth/4 and ONE bias unit is FOUR view units — derived from game data, NOT read back from a GTE control register. It is opt-in (known=false) so the two pre-existing callers of meshQuadRecordsEmit (fx_dust, narration_swirl) are unchanged: their bias arguments are not RE'd and claiming them would be jumping ahead of the RE. PROVEN TO DRAW with a producer-disabled negative control: two Release binaries from the isolated tree psx/scratch-plumeab/T2, distinct md5s (ON 9f356400540caf2efafb6d671a777dc6 / OFF e3920b921a23d5d70dac8bd1902b40bf), identical except the one dispatch branch; same replay replays/bugs/bucket-softlock.pad, headless, PSXPORT_GATE=1 pc_render, PSXPORT_PRESENT_SHOT_AT at 960x720. IN THE PRODUCER'S OWN ACTIVE WINDOW (plumefx says f252-f263): present 254 = 675 changed px of 691200 bbox x[465,515] y[273,299]; present 258 = 3267 px bbox x[432,545] y[261,368]; present 262 = 828 px bbox x[432,551] y[291,341]. NEGATIVE CONTROL OUTSIDE it: presents 300 and 320 = 0 changed px, and the producer is not called there. LEG PROOF IN BAND: 24 plumefx lines ON, 0 OFF. CONTAINMENT, added after plumefx was extended to report the producer's OWN emitted screen box and the gate was re-run against an OFF leg rebuilt from the same source (ON 4b58b00cb9d3cf30d582f87693327c39 / OFF b99520b74d2268a65bce5ebdb5e86ab3, identical 675/3267/828/0/0 result): scaled by ires 3, 100% of the f254 diff, 100% of the f262 diff and 99.72% of the f258 diff fall inside the box the producer predicted. THE 9 PIXELS THAT DO NOT are one native pixel (146,122) that goes pale yellow (206,206,107) OFF -> dark brown (107,74,49) ON, outside the plume's footprint: an ordering/depth-coincidence flip caused by the extra draws (kanban #74's class), UN-ROOT-CAUSED and recorded rather than smoothed over. REACHABILITY CENSUS, all 17 replays at 900 frames: SEVEN reach the producer with 24 calls each (bucket-softlock, house-on-the-point, save-prompt-black-screen, seesaw-weight, sequence-softlock-2, title-options-page, walk-dust-puff); the other TEN reach it 0 times (general-session, short-session, start-mash-smoke, dark-screen-repro, ingame-item-menu, ingame-options-page, save-sign-softlock, weapon-impact-bucket, hut-entry-alt, hut-entry-door-freeze); every run exit 0 with zero abort/FATAL/recomp-MISS, and `quads=0` never occurred. BOOT GATE: newgame + run 400 headless reaches frame=440 stage=8010637C sm48=2 both before and after this change. The diff mask is one connected radial cluster of white/yellow spikes around Tomba's head (scratch/plume/evidence/present_258_*.png in that tree), absent in the OFF crop. WHY ported-unverified: (a) no USER eyeball; (b) the controller's SECOND half — when node+3 is 0x14 or 0x15 it hands the list at node+0x34 to the SPRITE writer FUN_80027A4C — is NOT ported; every call observed carries subtype 0x07, so that branch is unreached rather than broken, and the plumefx line tags it [sprite-half reached — NOT PORTED] if a scene ever takes it; (c) no cross-check against psx_render, because the two legs' present-frame timelines are offset (the psx_render leg skips the OP FMV) and aligning them was not attempted. Death condition for verified: a USER eyeball plus a capture that reaches subtype 0x14/0x15.

## render-producer-charge-starburst
- **scope:** FUN_8002A834 — ten-copy weapon-charge starburst emitted through shared packed-mesh writer FUN_80027768
- **status:** verified
- **order:** 52
- **owner:** Render::swingStarburstRender (game/render/fx_swing.cpp)
- **notes:** PORTED + TRUE-ORACLE VERIFIED 2026-08-21. The type-0x20 display walk dispatches the MAIN.EXE controller by node+0x18. The producer reads all ten {angleX,angleY,angleZ,uniformScale} records from node+0x50, the node world anchor at +0x2C, owner type-selected far colour at 0x800A1FC4, node sort bias at +0x32, and fixed mesh 0x8009FB0C; it rebuilds the controller transform and calls the existing packed-record decoder with IR0=0xFFF and zero U/CLUT bias. It does not read GTE registers, guest packets, OT state, or execute/tap the shared writer. replays/bugs/weapon-charge-starburst.pad holds CIRCLE f620-1000. Fresh SBS oracle run: B reports PURE-ORACLE(interp+softGPU); native producer begins f667 with 10 copies/60 quads; saved pre-A to current-A deltas are 0 px at f650/f660 and 843/642/939/1001 px at f670/f680/f690/f700 in the starburst footprint. B is byte-identical before/after at all six frames, retaining the opposite answer. REPIN VERIFIED against definitive psxport 692b9b20 after the preliminary substrate re-emission and a final clean Clang 22.1.8 rebuild: all 38 producer lines and all six final-pin A/B capture pairs are byte-identical to the retained 9f run.

## render-producer-impact-plume-288ac
- **scope:** FUN_800288AC — the packed-mesh half of composite weapon-impact renderer FUN_80033080, also installed directly as a type-0x20 node render function
- **status:** verified
- **order:** 53
- **owner:** Render::impactPlumeRender (game/render/fx_impact.cpp)
- **notes:** PORTED + TRUE-SOFTWARE-ORACLE VERIFIED 2026-08-21. FUN_800288AC reads the current four-byte animation record at node+0x3C, builds Math::rotmat from node+0x48/4A/4C, column-scales by record bytes 0..2 <<2, anchors at node+0x2C/2E/30, and calls packed-mesh writer FUN_80027768 once with the node's s16 sort bias, u8 U scroll, black far colour, and record-attribute-selected depth cue or CLUT-row bias. The display-pass producer rebuilds those inputs under the native lerped camera; it reads no GTE state, scratchpad transform, guest packet, or generated-body output. The type-0x20 walk dispatches it for both direct rfn 0x800288AC and composite rfn 0x80033080, preserving the latter's sprite-then-mesh order. On weapon-impact-bucket.pad, pane B reports PURE-ORACLE(interp+softGPU) and is byte-identical before/after at all eight samples. Native A gains 588/826/492/206 pixels at f652/f654/f656/f658, entirely inside the producer's own screen box; both panes visibly contain the blue-white diagonal plume at f656. bucket-softlock.pad independently reaches direct 0x800288AC nodes from f298 through f332. Evidence: scratch/logs/c15_{pre,post}.log, scratch/logs/c15_standalone.log.

## render-fallback-water-jet-guest-gte
- **scope:** FUN_8013D454 non-zero-mode A00 water-jet mesh branch only; shared writer FUN_80027768 remains generated for every caller
- **status:** hack
- **order:** 54
- **owner:** game/render/guest_gte_water_jet.cpp
- **notes:** EXPLICIT USER-AUTHORISED FALLBACK 2026-08-21, not a native-producer milestone. The controller and writer execute their untouched generated bodies. During only the `0x8013D454` scope, the host validates and replays the exact newly written guest GT4 packet words at logic time; every packet must resolve all four packet-addressed guest GTE depths or the port aborts. The queued items carry integer SXY (`has_xyf=0`), so fps60 presents them verbatim and never display-pass interpolates them. No host transform, scale, anchor, camera or depth is reconstructed or guessed. `walk-dust-puff.pad` f460/470/480/490/510/520: two packets and 8/8 depth hits per call, zero miss/stale; native A gains 747/495/1020/1019/1095/791 pixels. f450/f500 are exact no-call/no-pixel negative controls. True-software-oracle B is byte-identical at all eight samples and identifies `PURE-ORACLE(interp+softGPU)`. The #15 impact replay remains byte-identical in both panes and never raises this scope. DEBT: `render-producer-effect-mesh-family` still counts `0x8013D454` among its seventeen absent native producers. Death condition: delete this module/installs when a controller-state display producer for both non-zero modes passes the same oracle/opposite-answer gates. Never widen the scope to another FUN_80027768 caller.

## pause-menu-chrome
- **scope:** in-game pause/item menu display producer (FUN_800346BC controller + FUN_8007E1B8/FUN_8007E6DC UI leaves)
- **status:** verified
- **owner:** game/ui/pause_menu.cpp (class PauseMenu)
- **notes:** kanban #21. Scoped leaf tap: gen bodies untouched, quads re-derived host-side at RQ_OVERLAY, ordered by the guest's own OT bucket (descending, LIFO within a bucket) rather than call order. Gate: 0/76800 differing pixels vs the psx_render leg with the menu open; 0/76800 field-HUD regression from the emitUiFt4 back-to-front flip.

## world-line-rope
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_line.cpp
- **notes:** FUN_8013DD34 shared rope leaf + its three callers (FUN_8013E9D8 anchor rope, FUN_8013EA64 8-point chain, FUN_80122974 4-mode tether incl. the 8-segment fishing line), ported as native producers; segment->quad expansion in the producer, queue stays quads-only. Gates: stroke count parity with the lineprim census, 130px/69px A/B isolation, fps60 lerp 5/5, SBS no new divergence.

## script-interp-advance
- **scope:** ScriptInterp::loadNextEntry + op04SceneFlagRendezvous (guest FUN_80040E54 / FUN_8004201C)
- **status:** verified
- **owner:** game/scene/script_interp.cpp
- **notes:** The cutscene interpreter's last two unowned links. Equivalence proven before use: full 2MB guest RAM at f2600 on replays/bugs/sequence-softlock-2.pad byte-identical to the pre-port run; SBS full green to f41280. loadNextEntry MUST install with a setter (gen_func_80040FA0 reaches it by a direct jal; rec_dispatch never sees it). Owning these is what exposed kanban #60.

## render-subpart-walk
- **status:** verified
- **notes:** Render::subPartWalk (FUN_8003F174): per-sub-part transform + geomblk submit; port_check PASS; wired with setter. LIVE (ovhit native=139 on the bucket capture). 2026-07-28: gained a DISPLAY-PASS half — Render::subPartCapture (subpart_capture.cpp) re-derives each sub-part's prims from its own geomblk + transform and pushes WqRecs, with Render::mSubPartDrawSuppress stopping the guest-time submit from also drawing them. That closed kanban #64: a text-label character's glyph cmd and its plank sub are the SAME pointer, that shared transform moves 10-15 units/axis every logic frame, and only the glyph half had a record — so letters lerped to the midpoint while their planks held the real-frame position. Verified 1740 same-frame glyph/plank objT pairs AGREE, 0 differ; picture-neutral at 26/76800 px on frame 240 of bucket-softlock.pad.

## fx-ring-sprite-110c14
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_sprite.cpp
- **notes:** Render::fxRingSpriteRender (FUN_80110C14, A0D overlay, area 13): 21-item orbiting sprite ring, the 6th FUN_80027A4C-family member and the first drawing MANY sprites from ONE node. RE'd from ov_a0d_gen_80110C14 (per-item table node+0x34, angle 1024+97i, radius (phase*spread>>5)+s16 node+0x32, per-item bob rsin(phase<<4)*bob>>12, gate FUN_800317CC(-50), record list 0x8009D5FC+(phase&0xF0) from a 16-slot MAIN.EXE bank, clut 0x8009D5F8). VERIFIED ON PIXELS: area 13 warp, skip 200 -> ON vs producer-removed OFF leg = 202 px differ at x[2..75] y[151..211], matching the producer's own logged screen extent; OFF leg 0 emissions and nofx names 80110C14, ON leg 610 emissions drawn=21/21. Legs from one source revision, distinct binary md5s, object-swap relink (shared source never edited).

## fx-particle-field-10c7f4
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_sprite.cpp
- **notes:** Render::fxParticleFieldRender (FUN_8010C7F4, A0L overlay, area 21): 64-particle wind-blown field, the only four-corner-writer member that DRIVES the depth cue (IR0 = Trig::vecLen(particle - ref) >> 3 against a BLACK far colour, so the field fades with range). LCG seed 0x12D687, multiplier at 0x80115894, THREE steps per particle (X pre-step, Y after 1st, Z after 2nd), value & 0x3FFF - 8192 off the base at 0x1F800160/62/64; wind drift = doubled rcos/rsin of the angle at 0x800E7ED6 times 10x node+0x50 >> 12, added to X and Z before the mask; gate FUN_800317CC(0); scale = published MAC0 << 1; record list cycles i&3 over four pointers at 0x80109068. VERIFIED ON PIXELS: area 21 warp + skip 600, ON vs producer-removed OFF leg = 2069 px differ at x[67..182] y[64..193], rendering as soft grey mist wisps against the black sky; 554 emissions drawn=32/64; 0x8010C7F4 dropped off the nofx census. Dependency Trig::vecLen (FUN_80078240) ported as a static. NON-REGRESSION: emitAnimQuadRecords gained ir0/farColour with identity defaults - walk-dust-puff.pad frame is byte-identical to the pre-change binary.

## fx-motion-trail-1113b4
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_trail.cpp
- **notes:** Render::fxMotionTrailRender (FUN_801113B4 -> FUN_80110B00, A03 overlay, area 3): screen-space ADDITIVE motion trail. 11-slot screen-position history at node+0x3C ({s16 x, s16 y}, (0,0) = unfilled), 10 joints, each drawn as 4 quads — a +/-1x core (ramp colour on the centre line, BLACK at the outer edge) inside a +/-4x halo at per-byte-halved brightness; 11-entry colour ramp at 0x80108FDC; perpendicular = ratan2(dy,dx)+1024 through rcos/rsin, (v*2+2048)>>12; joint rule keys on the LOOP COUNTER so a suppressed segment still advances the joint state; degeneracy history suppresses the segment ending at a null/duplicate point AND the next one. NOT a sprite-family member (no GTE/DQA/projection) — SpriteAnchor deliberately unused. Read-only: the guest's pool-room gate is not reproduced because this producer allocates nothing. VERIFIED ON PIXELS: area 3 warp + skip 600, ON vs producer-removed OFF leg = 4690 px differ at x[195..318] y[112..239], rendering as a coherent tapered glowing streak; 1140 emissions; 0x801113B4 dropped off the nofx census leaving only terrain + widescreen margin. Overlay guard needed and validated: 0x801113B4 exists in BOTH A03 (sp-=24, 0x27BDFFE8) and A0B (sp-=40), so the first-instruction check disambiguates them.

## fx-dot-haze-1110bc
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_dotfield.cpp
- **notes:** Render::fxDotFieldRender (FUN_801110BC, A0B overlay, area 11): the camera-following DOT HAZE — visually the area's ambient SNOW. 513 opaque white specks on a wrapping 2048-unit world lattice keyed to node+0x2C/2E/30, cube centred half a camera-forward step ahead of the eye; size 2x2 when SZ3 < 1536 else 1x1; LCG seed at node+0x50 (READ ONLY, never written back) with the multiplier at 0x8011C030, three steps per particle, particle 0 taking the RAW seed and one extra step whose value is never read. NOT a sprite-family member (no DQA, own two gates: GTE-FLAG/near-plane and an unsigned 0<=SX<320 screen clip with NO Y test). DELIBERATE DIVERGENCE: the guest prepends every dot into ONE fixed OT bucket (256) with no sorting; this producer gives each dot its real projected depth instead (engine owns ordering). VERIFIED ON PIXELS: area 11 warp + skip 600, ON vs producer-removed OFF = 330 px differ across x[0..319] y[12..227], drawing as snow over the night scene; 1196 emissions, 415/513 dots passing the gates; 0x801110BC dropped off the nofx census. NOTE: this was mis-scoped as the HARDEST sweep target on its callee list (matrix pipeline + raw RTPT); 0x80084660/690 are just SetRotMatrix/SetTransMatrix and the RTPS folds onto projComposeObjectHost(identity, origin) — no matrix helper and no gen body were needed.

## fx-backdrop-plane-110ca4
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_backdrop_plane.cpp
- **notes:** Render::fxBackdropPlaneRender (FUN_80110CA4, A0E overlay, area 14): the WATERFALL BACKDROP — kanban #48. Grid A = 7 rows x 10 cols of 1200x2400 model-unit quads at local Z=0 forming a 12000x16800 wall, V MIRRORED about y=0 with the lower half tinted 0x00201000 (the reflection) and a bright 0x00DFDFDF seam row at y==0; Grid B = 10 quads on the local Y=0 plane out to Z=1200, ADDITIVE, the glow band along the seam. U/V scroll from the frame tick at 0x1F80017C (U one 64-texel tile every 2 ticks, V every 8). Transform = projComposeObjectHost(MeshQuads::rotmat(node+0x48/4A/4C), node+0x2C/2E/30). Screen gate reproduced as the guest's TWO SEPARATE 'any vertex passes' tests (X<321, Y<241) read unsigned. DELIBERATE DIVERGENCE: the guest log-compresses AVSZ4 into an OT key and adds a ROW BIAS (+30 upper / +40 lower) plus a 2045 clamp — authored painter's-algorithm order; this producer uses real projected depth instead. VERIFIED ON PIXELS: area 14 warp + skip 600, ON vs producer-removed OFF = 27904 px differ at x[0..208] y[12..227], rendering as the waterfall wall plus its reflection; 1196 emissions, gridA 33/70 and gridB 7/10 passing the gates; 0x80110CA4 dropped off the nofx census. INCOMPLETE BY DESIGN: this is HALF the guest render fn — it tail-calls 0x801104D0 (440 gen lines, sprite family) with the same node, and that half is NOT ported.

## fx-rain-lines-116904
- **scope:** render
- **status:** verified
- **owner:** game/render/fx_motes.cpp + fx_motes.h
- **notes:** Render::fxMoteStreakRender (FUN_80116904, A08 overlay, area 8): 32 world motes drawn as doubled-length motion STREAKS — visually the area's RAIN. tail = 2*prev - cur, white head fading to mid-grey tail. LCG seeded at node+0x50 (never written back), multiplier 0x801450D8, THREE steps per mote with each axis reading the seed BEFORE its replacement; positions masked to 11 bits inside a 2048-unit cube re-centred each frame on (camera eye - 1024 + half the camera forward row); per-axis bit-11 XOR against last frame's cube base (node+0x48/4A/4C) suppresses the streak on the frame a mote wraps. NOT a sprite-family member, and its OT gate is NOT SpriteAnchor::otKeyInRange — same log map but WITHOUT the k<4 pre-clamp, so it REJECTS a near range otKeyInRange accepts (kGateNoPreClamp). ONE-FRAME-DIFFERENTIAL: the previous screen positions come from a HOST-SIDE shadow (class MoteStreaks, fx_motes.h) rotated once per LOGIC frame on gpu.s_frame, the EffectLerp idiom — the guest's own array at 0x801485E8 is NOT read because whether it holds this or last frame's values depends on when the substrate walk ran. VERIFIED ON PIXELS: area 8 warp + skip 600, ON vs producer-removed OFF = 1432 px differ across x[29..319] y[0..239], rendering as rain; 1196 emissions, 32/32 streaks with prev=yes, 0 wrapped; 0x80116904 dropped off the nofx census.

## libgpu-setdrawenv-81fb0
- **scope:** render
- **status:** verified
- **owner:** LibgpuDrawEnv::setDrawEnv (game/render/libgpu_draw_env.cpp)
- **notes:** libgpu SetDrawEnv(DR_ENV*, DRAWENV*) at 0x80081FB0 — compiles the frame's drawing environment into the 6-or-9-word GP0 packet PutDrawEnv (0x800815D0) / DrawOTagEnv (0x800816A0) send to the GPU DMA; ~6000 dispatches, 2x/frame. Identity from the callers (Ghidra scratch/decomp/setdrawenv_81fb0.c) + the five already-owned word builders (0xE3/0xE4/0xE5/0xE1/0xE2). True extent [0x80081FB0,0x80082220), 156 instr, confirmed three ways (disas jr-ra at 0x80082218 + delay slot; 0x80082220 is a call target of this function; port_gen live extent 12834-12980 of shard_4.c with no folded sibling). GATED: port_check PASS, build clean, SBS full 0-diff f0..f1800, ovhit native=300/300 frames single-core. See docs/parity-map.md libgpu-setdrawenv-81fb0.

## render-tap-precomposed-matrix
- **scope:** pc_render producers that recover a transform by FACTORING the guest's pre-composed matrix against the scene camera (wq_factor_world)
- **status:** verified
- **owner:** game/render/render_internal.h:88 (wq_factor_world); callers text_label.cpp:143, render_walk.cpp:140, widescreen_margin_quad.cpp:317, quad_rtpt_submit.cpp:245
- **notes:** RESOLVED 2026-08-04. wq_factor_world and wq_read_matrix are DELETED from render_internal.h, and with them every caller's capture: text_label.cpp (glyph WqRecs), subpart_capture.cpp (whole file deleted, plus the mSubPartDrawSuppress handover in submit.cpp that only existed to pair with it), Render::perObjFlushPreComposed (deleted; pre-composed-matrix node types now get NO generic native mesh flush), quad_rtpt_submit.cpp and widescreen_margin_quad.cpp (deleted with the GTE-register tap). The false comment claiming the round trip was 'Exact at the endpoints for ANY CR content' went with it. Death condition met: zero references to wq_factor_world/wq_read_matrix in the tree. Cost, reported honestly: the item-announcement banner and every other pre-composed-matrix node class now have NO pc_render picture — measured, node 800FB218 went from 196 prims per present to 0, and the banner is absent from the screenshot. That is the USER's stated preference over a tap.

## render-producer-cube-text-banner
- **scope:** the item-announcement / cube-text banner (node class FUN_8003AD48, drawn by FUN_80039F4C textLabelEmit + FUN_8003F174 subPartWalk) has NO pc_render picture
- **status:** verified
- **notes:** RESOLVED 2026-08-04, same day it was opened. game/render/cube_text_banner.cpp — CubeTextBanner::render, called from the native object walk for pre-composed-matrix nodes and self-filtering on node+0x1C == 0x8003AD48 (structural identity, not a tag). It rebuilds each glyph's transform from the fields the (already native) behaviour owns — rec.R = node.R * rotmat(rec+0x08), rec.T = node.R * (rec+0,+2,+4) + (node+0x2E,+0x32,+0x36), i.e. NodeXform::propagateRotmat's own math recomputed from its inputs — and projects it as a VIEW-SPACE transform with ofx/ofy/H alone. The camera does not enter the arithmetic, so a camera-dependent residue is structurally impossible, not merely small. It draws BOTH halves (glyph quad + the record's plank geomblk) from that one transform, which also makes kanban #64's glyph-vs-plank drift impossible; subpart_walk.cpp hands the guest-time draw over for exactly this node class (host-side skip only). GATE, same instrument/object/window as the defect measurement (tools/preseqobj_check.py --node): camera panning, mean |dX| 1.48 px with 12/12 sign alternations BEFORE -> 0.15 px with 0/16 alternations AFTER; camera still, 0.00 px exactly. All residual motion now traces to the banner's own bounce-out animation (measured dY -14.9,-13.0,-11.1,-9.5,-7.2,-5.6,-3.7 px == the guest's rec+0x12 gravity integration, -256 stepping +32/frame). Guest RAM + scratchpad byte-identical to the pre-change binary over a spawn+40-frame run. Hermetic gate: PSXPORT_SELFTEST=cubetext asserts the output is byte-identical across two cameras, with a negative control proving the comparator sees an 81.2 px change for a camera-composed projection of the same points.

## render-tap-gte-registers
- **scope:** pc_render producers that source an object transform from GTE HARDWARE REGISTERS after the substrate ran
- **status:** verified
- **owner:** game/render/quad_rtpt_submit.cpp, game/render/widescreen_margin_quad.cpp (deleted-tap audit sites)
- **notes:** RESOLVED 2026-08-05. No gte_read_ctrl(0..7) transform-recovery survives in any pc_render producer. Two hits remain repo-wide and both are legitimate: a COMMENT in quad_rtpt_submit.cpp describing the deleted tap, and Math::applyMatlv (a byte-exact port of guest FUN_80084220/MVMVA reading the matrix the GUEST loaded via CTC2 and writing guest memory — the guest's own coprocessor use, not a picture tap). perobj_billboard.cpp's remaining 12 gte_read_ are all guest-side emitter port feeding c->mem_w32; the CR0-4 rotation / CR5-7 anchor capture that WAS the tap is gone. swing_fx, fx_mesh and mesh_emit_tap deleted outright, no tombstones. NOTE fx_sprite_anchored.cpp and fx_sprite_swarm.cpp were a FALSE POSITIVE in the brief: they install via overrides::install against guest 0x80027CB4/0x80027E5C/0x800281EC, so they are byte-exact ports of the GUEST emitters and deleting their GTE reads would have broken the port. BREAK-FIRST cost, measured: 22009 preseqobj prims over 18 node keys -> 21889 over 15; exactly 3 keys and 120 prims gone, 15 other keys byte-identical. Visibly: an orange food pickup on the wooden fence rail, and on another replay the apple on the barrel. FxMesh/SwingFx emitted 472 quads but contributed 0 pixels and 0 preseqobj records in the TAPPED leg — already dead for the picture, so deleting them cost nothing. HONEST GATE: the camera-pan number is a NON-REGRESSION not a proof (1.10px/0-of-16 tapped vs 1.12px/0-of-16 rebuilt) — the instrument gave 0/16 on the tapped leg too, so it never showed the failing answer and cannot show it fixed. The claim rests on STRUCTURE: the record carries no camera term, so a camera-dependent residue is impossible by construction. The deleted FUN_8013CDD4 picture is no longer blank: render-producer-margin-quad closed it on 2026-08-22 with a persistent-state display producer, without restoring any tap. The submitQuad caller classes remain tracked separately by render-producer-submitquad-classes.

## fx-line-emitter-e08c
- **scope:** 0x8013E08C — the unowned LINE/strip emitter beside the owned fx_line producers
- **status:** verified
- **owner:** game/render/fx_line.cpp (Render::shockwaveRingRender)
- **notes:** PORTED 2026-07-28 as Render::shockwaveRingRender (game/render/fx_line.cpp), whitelisted in fieldObjectsRender behind the overlay first-instruction guard (0x27BDFFB8). FIXED AND PIXEL-VERIFIED 2026-08-06 — it had been drawing NOTHING since it landed (claim C036 -> C037), from TWO independent bugs, either fatal alone: (1) the object translation was read from node+0x4E, the ROPE/TETHER node family's layout, while FUN_8013E08C hands the packed SVECTOR at node+0x2C to 0x80084220 (X@0x2C, Y@0x2E, Z@0x30) — and on a ring node 0x50 is the SCALE animator, so the port's Y was the ring's own radius and its X/Z came from unrelated fields, putting it near the world origin while the camera sat at (10264,-2124,3979); (2) Robj was divided by 4096 although projComposeObjectHost takes 1.3.12 (4096 = identity), collapsing every ring to a single point. Full RE re-derived against the emitter's own instruction stream (generated/ov_a00_shard_0.c ov_a00_gen_8013E08C): uniform scale diag((s16)node+0x50 << 4) in 1.3.12; colour v = 0x80 - ((node+0x50 - 0x14)*0x80)/200; matMul 0x80084110 leaves the CAMERA rotation in GTE CR0-4 so 0x80084220 is an MVMVA of camera x nodePos, + the camera translation at 0x1F80010C — i.e. exactly projComposeObjectHost(diag(scale), nodePos), no GTE; 15-point radius-256 XZ-plane ring at 0x8014C780 walked as 7 overlapping 3-point spans. ALSO CORRECTED: each span is emitted TWICE with its OWN DR_MODE (0x80083DE0 tpage 53 -> blend bits 1 = B+F additive, unshifted; tpage 85 -> blend bits 2 = B-F subtractive, at (+2,+1), drawn underneath) — an embossed highlight+shadow pair, not a 2px stroke; the producer had both copies on blend 3. NOT reproduced deliberately: the guest's OT-index bias by (s16)node+0x32 (the native queue orders by real depth). GATE: three separately-built binaries (distinct md5s, exit-0 checked, in-band leg proof), isolated tree, replays/bugs/bucket-softlock.pad, pc_render headless, PSXPORT_PRESENT_SHOT_AT at presents 275/280/287/320/340/355 — all INSIDE the producer's own f270..f358 window — fixed-vs-deleted = 450/657/909/2151/1602/2232 changed px of 691200, every diff mask a single closed ellipse outline with no stray region; NEGATIVE CONTROL: the shipped producer vs the same deleted leg on the same presents = 0/6. Native projected screen box now tracks the guest's own lineprim packet vertices to ~1px on 8 sampled frames. Two traps still worth keeping: the vertex loads are LWC2 (Ghidra shows them as opaque setCopReg) and the sibling 0x8002ECD8's tail is mis-rendered as a call to Trig::rsin.

## world-line-ring-shadow
- **scope:** render
- **status:** verified
- **deps:** world-line-rope
- **owner:** game/render/fx_line.cpp (Render::shockwaveRingRender)
- **notes:** SUPERSEDED BY step fx-line-emitter-e08c — same address, and that step now carries the RE and the pixel gate. Kept as a pointer because the missing-layer list and kanban #56 both cite this id. THE NAME IS WRONG AND IS NOW SETTLED: FUN_8013E08C is an EXPANDING SHOCKWAVE RING, not a ground ring shadow — measured 2026-08-06 on replays/bugs/bucket-softlock.pad, it grows from ~6px across at f270 to ~80px at f357 while its grey fades 122 -> 13, three instances over f270..f358. The two open items this step recorded are both closed: (1) which visual it is — settled above; (2) the 152-vs-76 factor of 2 is NOT a bug and is no longer an inference: the 152 producer calls are exactly 76 distinct (frame,node) pairs seen TWICE, one per PRESENT, because fps60 re-renders the field object walk for the interpolated present — with fps60=0 in psxport_settings.ini the same replay logs 76, matching the guest's 1064 packets / 14 per call. The producer itself was BROKEN (drew zero pixels) until 2026-08-06; see fx-line-emitter-e08c for the root cause and the gate.

## fx-sprite-emitter-b3a4
- **scope:** 0x8002B3A4 — the 5th 0x80027A4C sprite-family emitter, rotation-composed
- **status:** verified
- **owner:** game/render/fx_sprite.cpp; shared fixed-point transform math in game/render/mesh_quads.h
- **notes:** VERIFIED 2026-08-21 with the real-input `replays/bugs/stun-stars.pad` and a live

## render-compose-tint-gate
- **status:** ported-unverified
- **notes:** Render::composeTintGate (FUN_8003EF9C): per-type render gate, port_check PASS, wired via overrides::install with setter. Pool-snapshot idiom: emits geometry then colour-adds over exactly the primitives just emitted. Cold on the field/dialog replay - needs a scene that uses render mode 2.

## render-shared-transform-walk
- **status:** ported-unverified
- **notes:** Render::sharedTransformWalk (FUN_8003F07C): rigid-node sibling of subPartWalk - loads ONE view transform from scratchpad 0x1F8000F8 then submits every sub-part under it. port_check PASS, wired with setter. Cold on the field/dialog replay (its caller composeTintGate is also cold there).

## render-panel-fill
- **status:** ported-unverified
- **notes:** Panel::fillQuad (FUN_8004FFB4): the 9-slice panel fill quad, hottest unowned render fn on the field path. port_gen byte-faithful, port_check PASS, wired with setter, LIVE at 505 hits with the frame unchanged. READABILITY PASS PENDING - still in register form. Note game/ui/panel.cpp:185 calls gen_ directly so the existing tap is not intercepted.

## fx-emitter-ecd8-e680
- **scope:** The 0x8002ECD8 + 0x8002E680 effect emitter pair (type-0x20 node render fn, no producer)
- **status:** ported-unverified
- **owner:** game/render/fx_ring.cpp
- **notes:** PORTED 2026-07-28 as Render::impactRingRender + Render::impactAnnulusDraw. The pair is fully RE'd; the vertex layout that blocked it was settled with tools/mips_trace.py. FUN_8002E680 is a screen-space ANNULUS rasteriser: 5 authored wedge angles (0x66 then the s16 table at 0x800A20A8 = 204/307/409/512, i.e. 9/18/27/36/45 degrees), each segment a gouraud quad (GP0 0x3A) with v0/v2 on the inner radius carrying a2 and v1/v3 on the outer carrying a3 (a radial half->full gradient, since the caller passes a2 = a3>>1 & 0x7F7F7F), replicated over its EIGHT dihedral images; segment 0 emits only 4 because its span is mirror-symmetric. 4 + 4*8 = 36 quads, confirmed by the guest's 0x144-word pool advance. It ends by linking a SetDrawMode tpage 0x35 prim in front, so the blend is ABR 1 = additive. FUN_8002ECD8 is the node half: centre/scale either the fixed HUD (32,32)/1.0/OT-4 when node+3==0x91, or the anchor at node+0x2C RTPS'd with DQA=6 (the family's SpriteAnchor::baseScale + otKeyInRange), and radii base=48+32*sin(a), outer=ceil(base/2), inner=ceil((base-32*cos(a))/2) from the single animator byte node+5, halved again when the s16 at 0x800E7FFE is negative. Whitelisted on rfn 0x8002ECD8 in fieldObjectsRender (MAIN.EXE-resident, no overlay guard needed). STILL RUNTIME-UNVERIFIED: it did NOT fire on replays/bugs/weapon-impact-bucket.pad (that impact uses 0x80033080), so no replay in the library reaches it yet. impactAnnulusDraw is deliberately separate from the node half because the guest leaf has ELEVEN call sites (MAIN.EXE + the A01/A06/A08/A0J overlays) — the other ten can be ported onto this same producer.

## fx-backdrop-sparks-1104d0
- **scope:** render
- **status:** ported-unverified
- **owner:** game/render/fx_backdrop_plane.cpp
- **notes:** Render::fxBackdropSparkRender (FUN_801104D0, A0E overlay, area 14) — the tail half FUN_80110CA4 tail-calls (kanban #67), now called from fxBackdropPlaneRender exactly as the guest tail-calls it. A fixed 200-slot sprite-particle pool: 0x8012686C + i*4 is the slot's record list AND its live flag (non-zero = live), 0x80125BEC + i*8 is {u16 x,y,z}, 0x8012622C + i*8 is velocity, clut|tpage at 0x8011B224, gate bias -50, DQA 6, IR0 = 0. WHY THE PORT IS SMALL WHERE THE GEN BODY IS 441 LINES: that body SIMULATES AND DRAWS — it integrates pos += vel, adds 25 to vel.y for gravity, writes both back, and its bulk is the SPAWN state machine where all 34 of its PRNG draws live. A read-only producer reproduces NONE of that; the guest's own body keeps the pool simulated underneath, so this reads slot state and emits. It therefore needs no GuestRngMirror — the randomness is upstream of the state we read. STATUS ported-unverified, and the reason is measured not assumed: in the area-14 capture (warp 14 + skip 600) the pool reads live=0/200, so there is nothing to draw and the pixel delta against the grids-only build is 0. That is an EMPTY POOL, not a dead producer — the fxplane 'sparks ... live=N drawn=M/200' line distinguishes the two, which is instrument I022's lesson applied. Verify when a scene populates the pool.

## render-producer-beam-b704
- **scope:** FUN_8003B704 — the BEAM / see-saw ribbon between the scene's tracked anchor *(0x800E7F5C) and the node's own world position (unported-render-inventory R2, first of the three submitQuad caller classes)
- **status:** ported-unverified
- **owner:** Render::beamQuadRender (game/render/fx_beam.cpp)
- **notes:** PORTED 2026-08-06. RE from ground truth generated/shard_0.c gen_func_8003B704 (no Ghidra needed - the emitter has no COP2 op of its own). THE CR-CONTRACT QUESTION THAT BLOCKED THIS ROW IS ANSWERED, and the answer is in the emitter itself: it calls func_80084660/func_80084690 (libgte SetRotMatrix/SetTransMatrix) with a0 = 0x1F8000F8 = the PURE CAMERA, overwriting whatever perObjRenderDispatch/billboardCompose1 left in CR0-7, immediately before building its corners. So its corners ARE WORLD SPACE and the native producer needs nothing but the node's own state + the native camera. Geometry: H = 0x14 * (cos a cos b, sin b, -sin a cos b) from node+0x68/+0x6A (+1024 on the polar when *(u8*)0x800E7FC6 < 4); span A=*(0x800E7F5C)+0x2C/30/34 (s32) to N=node+0x2E/32/36 (s16), split at the rounded-toward-zero midpoint when (s16)node+0x60 == 3; each span drawn as (P-H, Q-H, P+H, Q+H); U fixed [224,247], the two V rows from the 2-byte entry at 0x800A3B04[node+0x66]; GP0 code 0x2D (RAW - the colour word is never written), tpage half 5, clut 0x3E9F. Dispatch mirrors FUN_8003EEC0's own jump table at 0x80015000, re-read live (types 1 -> arm 0x8003EF30 always, 16 -> arm 0x8003EF40 gated on node+2==1; dumped from the running game, type 32 goes to 0x8003EF68 so no type-0x20 node is hidden by fieldObjectsRender's earlier continue). PROVEN TO DRAW with a negative control: two binaries identical except this producer (the NO-BEAM leg built in an isolated tree at psx/scratch-beamab/T2, both Release), same replay replays/bugs/weapon-impact-bucket.pad, headless; f652 84 changed pixels in bbox x[153,179] y[120,125], inside the producer's own reported screen bbox [145.9,117.3]..[188.8,129.3]; f646 26 px, f648 7 px, f650 0 px (the producer's own log says the span was degenerate A==N there, i.e. zero-area - the honest zero). NO other pixel in any of the four frames changed. WHY ported-unverified and not verified: only the kind!=3 single-span form and only the 0x8003EF30 arm were ever reached - the split (kind==3) form and the billboard arm (0x8003EF40, node+2==1) are UNEXERCISED across the whole 17-replay library, and no USER has eyeballed the layer. Reachability census (PSXPORT_DEBUG=beamfx, 900 frames each, all 17 replays): weapon-impact-bucket 52 producer calls, save-sign-softlock 42, seesaw-weight 28, walk-dust-puff 28, the other 13 replays 0 with the summary line carrying the denominator. Death condition for 'verified': a capture that drives the split form and the billboard arm, plus a USER eyeball.

## render-producer-margin-quad
- **scope:** FUN_8013CDD4's GT4 prop quads (drum/windmill caps) under pc_render
- **status:** ported-unverified
- **owner:** game/render/prop_quad.cpp Render::propQuadRender
- **notes:** PORTED 2026-08-22. Separate display-pass producer rebuilds the transform from persistent obj+44 anchor, obj+72 angles and node+0..2 authored scale bytes; shared MeshQuadStyle carries the RE'd U/CLUT/fog/tpage/semi policy through the one packed-record walker. No GTE/packet/OT/scratchpad/generated-body input. bucket-softlock headless: 3520 producer calls, 4326 native prims attributed to 0x8013CDD4 over 97 frames, native and live psx screenshots both show the prop assembly. User animation/visual eyeball remains; no claim of frame-exact pixel parity.

## render-camera-projconst-from-gte
- **scope:** the native camera's OFX/OFY/H are read from GTE control registers 24/25/26
- **status:** hack
- **owner:** game/render/scene_build.cpp:74-76
- **notes:** Registered 2026-08-05 by the agent that retired the object-transform taps, AGAINST ITS OWN WORK. scene_build.cpp fills the native camera's projection constants from gte_read_ctrl(24/25/26), and EVERY native producer inherits them via Fps60::sceneCam — INCLUDING the exemplar cube_text_banner.cpp. So 'zero gte_read in a native producer' was never fully true, including before today. Much weaker than an object transform (three per-frame scalars, no per-object residue) but it is the same banned shape. REAL FIX: derive OFX/OFY/H from the game's own camera/projection state. Death condition: no pc_render path reads gte_read_ctrl(24/25/26).

## render-producer-submitquad-classes
- **scope:** the FUN_8003B320 (submitQuad) caller classes with no pc_render picture — a00-overlay flame/rope emitter ~0x801341xx and case-188 particles. The THIRD, B704 beams, is now ported (render-producer-beam-b704)
- **status:** todo
- **absent:** the PICTURE for the two REMAINING caller classes (a00-overlay flame/rope emitter, case-188 particles) was DELETED 2026-08-04, not left unported — its GTE-register tap is banned by PROTOCOL.md (USER, absolute). Rebuild ONLY as a native producer reading each emitter's own world state.
- **notes:** OPENED 2026-08-04 by the tap-retirement pass. Their display-pass records were built from gte_read_ctrl(0..4)/(5+i) after the substrate's RTPT ran, then un-composed against the scene camera. Deleted per the USER's absolute rule. REAL FIX: port each emitter and draw from its own world state; there is no shared shortcut, which is exactly why the shared tap existed. 2026-08-06: B704 SPLIT OUT AND PORTED as render-producer-beam-b704, and it settled the CR-contract question this row recorded as open — B704 loads the PURE CAMERA (0x1F8000F8) into CR0-7 itself right before projecting, so its corners are world space. THE SAME IS *NOT* ESTABLISHED FOR THE OTHER TWO: case-188 (renderWalkCase188, render_walk_dispatch.cpp) loads CR0-7 from CASE188_SCR, and the a00 emitter from whatever it composes — each needs its own answer read out of its own body. CASE-188 IS ALSO UNREACHABLE IN THE CURRENT CAPTURE LIBRARY: its dispatch target 0x8003C188 is never taken in any of the 17 replays, so a port of it could not be picture-verified today (the same trap as fx-jet-mesh-sprite-10c1d8). And note the instrument caveat found on 2026-08-06: the  channel that would census those targets lives inside Render::renderWalk, which is an OVERRIDE, and under PSXPORT_GATE=1 every override runs its gen body — so  prints NOTHING in the standard measurement mode and its silence is not evidence.

## framework-dead-fps60bbswap
- **scope:** external/psxport game_iface.h carries a dead hook field fps60BbSwapPrev
- **status:** todo
- **notes:** OPENED 2026-08-04. Its only ever purpose was rotating Tomba!2's WqRec front/back buffers for the fps60 lerp; WqRec is deleted (it was the banned factored-transform record), so the hook rotates nothing in any of the three games and Tomba!2 must supply an empty function because fps60.cpp calls it unconditionally. FIX: delete the field + the call site. Cross-repo (spider1/spyro checkouts carry the same field), so it needs a coord claim and the operator to land it framework-first. Death condition: no fps60BbSwapPrev in psxport.

## fx-area4-ambient-13b118
- **scope:** render
- **status:** blocked
- **owner:** generated (unported)
- **notes:** FUN_8013B118 (A04 overlay, area 4). BLOCKED — every branch is gated OFF in the only reachable area-4 state, checked 2026-07-29 BEFORE writing code (warp 4 + skip 600, cross-checked against the older scratch/raw/c18_a4.bin): story phase 0x800E7EAA = 1 in both, so branch A (the 342-point field + sprite cluster) needs >= 44 and is not taken; the common tail needs phase in {2,3,4} and is not drawn; and node 0x800EDC90's fade at +0x58 = 4096 (fully faded) takes the branch that skips the mesh panels. So the fn draws NOTHING there and a port could not be pixel-verified — the same trap as FUN_8010C1D8 (instrument I022). TWO FURTHER BLOCKERS from the spec, independent of the gating: (1) the 342-point field lives in ov_a04_func_8013AD90, a 218-line raw GP0 tile emitter with its own LCG/RTPS/pool guard and NO analogue anywhere in game/render/ — its own producer card, not part of this port; (2) the mesh IR0 cue contains an Rng::next() dither and Rng::next() WRITES the guest seed at 0x80105EE8, so a read-only producer may not call it — that needs a host-side RNG mirror or a different cue source. UNBLOCKING: find a scene where the area-4 story phase is 2/3/4 (mesh + tail) or >= 44 (branch A), and note the phase is mirrored to 0x1F800207 only at scene setup, so poking one without the other desyncs the two reads of the same fork.

## fx-jet-mesh-sprite-10c1d8
- **scope:** render
- **status:** blocked
- **owner:** generated (unported)
- **notes:** FUN_8010C1D8 (A0L overlay, area 21). PREREQUISITES RESOLVED 2026-07-29 from a live area-21 RAM dump (scratch/raw/a21_dump.bin), BEFORE writing any code, because its verifier flagged both:
