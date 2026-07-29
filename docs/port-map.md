# RE + port frontier — is each step REAL (re-verified) or a HACK? (managed by tools/portmap.py)

The RE dependency chain. `## ` block per step. Work `portmap.py next`; kill `portmap.py hacks`.
Detail lives in docs/port-progress.md; this is the queryable real-vs-hack frontier.

**Status:** 23 verified · 13 ported-unverified · 1 todo · 3 blocked

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
- **notes:** PORTED + SBS-gated 2026-07-29 (50/50 identical, ovhit 7397/7397 balanced, port_check PASS). Status is ported-unverified NOT because the behaviour is unproven but because the body is still in port_gen REGISTER FORM — a byte-faithful c->r[] transcript, which CLAUDE.md is explicit is a transcript rather than a port and actively hides state forks. The READABILITY PASS is outstanding: typed lenses over the actor/other/anchor blocks (field map in docs/re/collision-resolve-23d48.md), named constants for the four outcome codes, and named control flow replacing the L_8002xxxx labels. Prove equivalence after renaming with port_check.py, which already PASSes on the draft. The stack hazard is HANDLED and must stay handled: the five callees are reached through their generated func_XXXX wrappers, never the Trig methods.

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

## render-compose-tint-gate
- **status:** ported-unverified
- **notes:** Render::composeTintGate (FUN_8003EF9C): per-type render gate, port_check PASS, wired via overrides::install with setter. Pool-snapshot idiom: emits geometry then colour-adds over exactly the primitives just emitted. Cold on the field/dialog replay - needs a scene that uses render mode 2.

## render-shared-transform-walk
- **status:** ported-unverified
- **notes:** Render::sharedTransformWalk (FUN_8003F07C): rigid-node sibling of subPartWalk - loads ONE view transform from scratchpad 0x1F8000F8 then submits every sub-part under it. port_check PASS, wired with setter. Cold on the field/dialog replay (its caller composeTintGate is also cold there).

## render-panel-fill
- **status:** ported-unverified
- **notes:** Panel::fillQuad (FUN_8004FFB4): the 9-slice panel fill quad, hottest unowned render fn on the field path. port_gen byte-faithful, port_check PASS, wired with setter, LIVE at 505 hits with the frame unchanged. READABILITY PASS PENDING - still in register form. Note game/ui/panel.cpp:185 calls gen_ directly so the existing tap is not intercepted.

## fx-line-emitter-e08c
- **scope:** 0x8013E08C — the unowned LINE/strip emitter beside the owned fx_line producers
- **status:** ported-unverified
- **notes:** PORTED 2026-07-28 as Render::shockwaveRingRender (game/render/fx_line.cpp), whitelisted in fieldObjectsRender behind the overlay first-instruction guard (0x27BDFFB8). Full RE in the file banner: uniform scale diag((s16)node+0x50 << 4), colour v = 0x80 - ((node+0x50 - 0x14)*0x80)/200 clamped, transform reduced algebraically to projComposeObjectHost(diag(scale), nodePos) so NO GTE is used, 15-point radius-256 XZ-plane ring at 0x8014C780 walked as 7 overlapping 3-point spans, each stroked TWICE with the guest's (+2,+1) offset. Two traps recorded there: the vertex loads are LWC2 (Ghidra shows them as opaque setCopReg) and the sibling 0x8002ECD8's tail is mis-rendered as a call to Trig::rsin.

## fx-sprite-emitter-b3a4
- **scope:** 0x8002B3A4 — the 5th 0x80027A4C sprite-family emitter, rotation-composed
- **status:** ported-unverified
- **notes:** PORTED 2026-07-28 as the FN_RINGROT branch of Render::fxSpriteRender (game/render/fx_sprite.cpp), whitelisted alongside the other sprite-family members in fieldObjectsRender.

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

## world-line-ring-shadow
- **scope:** render
- **status:** todo
- **deps:** world-line-rope
- **notes:** FUN_8013E08C: op-0x4A ground ring shadow, its own GTE loop over the 16-point circle at 0x8014C780 (sliding 3-point window), grey = 0x80-((nodeY-0x14)*0x80)/200, blends 1 and 2, node matrix at node+0x2C via FUN_80084220 + a diagonal scale from nodeY<<4. BLOCKED on RE of FUN_80084110/FUN_80084220.

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
