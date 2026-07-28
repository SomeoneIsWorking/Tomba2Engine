# RE + port frontier — is each step REAL (re-verified) or a HACK? (managed by tools/portmap.py)

The RE dependency chain. `## ` block per step. Work `portmap.py next`; kill `portmap.py hacks`.
Detail lives in docs/port-progress.md; this is the queryable real-vs-hack frontier.

**Status:** 17 verified · 10 ported-unverified · 1 todo · 1 blocked

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
- **status:** verified
- **order:** 44
- **owner:** game/render/fx_sprite.cpp
- **notes:** RESOLVED 2026-07-28, and the premise that filed it was WRONG (see falsified claim C010). FUN_800328EC is a 3-instruction wrapper — zero the depth cue at 0x1F800090, tail into FUN_8002847C — i.e. the SAME four-corner writer Render::fxAnimSpriteRender already reproduces. The gap was DISPATCH, not a producer. Its controllers carry a different node layout: anchor as three separate s16s at 0x2E/0x32/0x36, packed 8.8 scale pair at 0x60, animation-script pointer at 0x64, per-frame record table at 0x6C. Shared leaves: 0x800329E0(dqa) = scene-camera CRs + DQA/DQB, 0x800317CC(bias) = RTPS + the otKeyInRange gate publishing OT key/SXY2/MAC0 to 0x1F800080/0x1F80008C/0x1F800084 (the same contract fx_ring.cpp documents for FUN_8002ECD8, which inlines it). Ported as Render::altSpriteEmit + fxAltAnimSpriteRender (0x8012E868) + waterJetSpriteRender (0x8013D454's mode-0 branch), whitelisted behind first-instruction residency guards. VERIFIED: 228 emissions on walk-dust-puff and seesaw-weight; A/B with the two whitelist entries compiled out gives 419-936 px differing on 4 of 6 sampled frames (f510/f520 identical, the node is in its mesh mode there). The remaining callers 0x801346C0, 0x8013B118, 0x8010C1D8 and the two MAIN.EXE sites now need only a whitelist entry each plus their own scale rule.

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

## world-line-ring-shadow
- **scope:** render
- **status:** todo
- **deps:** world-line-rope
- **notes:** FUN_8013E08C: op-0x4A ground ring shadow, its own GTE loop over the 16-point circle at 0x8014C780 (sliding 3-point window), grey = 0x80-((nodeY-0x14)*0x80)/200, blends 1 and 2, node matrix at node+0x2C via FUN_80084220 + a diagonal scale from nodeY<<4. BLOCKED on RE of FUN_80084110/FUN_80084220.
