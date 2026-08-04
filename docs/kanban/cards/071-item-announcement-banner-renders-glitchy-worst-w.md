---
id: 71
title: Item-announcement banner renders glitchy, worst while the camera moves
status: done
labels: [render]
created: 2026-08-04
updated: 2026-08-04
evidence: docs/reference/issues/item-banner-glitch-2026-08-04.png
---

**2026-08-04:** USER-REPORTED 2026-08-04 with a screenshot: docs/reference/issues/item-banner-glitch-2026-08-04.png ('A Red Treasure Chest'). Each glyph is its own rounded tile. In the STILL, tiles sit at inconsistent vertical offsets (a stagger across the row) and the artwork inside each tile does not line up tile-to-tile — each shows a different slice of the stone/cliff texture. USER: 'it's glitchy when camera is moving but it's not something I can describe.' The motion-dependent part is UNCHARACTERISED and must be OBSERVED, not inferred from the still; the static defect above may be only part of it.

**2026-08-04:** CORRECTION 2026-08-04, from the USER, who is observing the running game — my earlier note on this card was WRONG and must not be relied on. I described the screenshot as showing inconsistent vertical tile offsets and misaligned tile artwork. It does not. USER: 'My screenshot isn't really evidence because it's a static show and it looks fine, the issue is more visible when animated and camera motion.' The still is REFERENCE — it identifies WHICH effect (the item-announcement banner, one rounded tile per glyph) — and is NOT evidence of the defect. The defect is ANIMATION- and CAMERA-MOTION-dependent and remains entirely UNCHARACTERISED. There is no known static symptom to look for. Anyone working this must reproduce it under motion and describe what they actually see; do not go looking for the misalignment I invented.

**2026-08-04:** USER CHARACTERISATION 2026-08-04: the effect is VIBRATING. That is the symptom — not misalignment, not missing geometry. A per-frame positional jitter of the banner (or of its tiles) that is invisible in any single frame and worst under camera motion. This is consistent with (but does NOT confirm) a transform that is not interpolated while the camera is: Tomba!2 runs fps60 interpolation over a 30Hz guest, and CLAUDE.md requires lerp at the actor-transform tier from state the native producer owns. Test that; do not assume it.

**2026-08-04:** USER 2026-08-04: 'I think it still happens without 60fps.' So the fps60 interpolation tier is RULED OUT as the sole cause — the lerp hypothesis (an un-lerped effect transform under a lerped camera) does not survive. User hedged with 'I think', so confirm with the measurement once the sandbox can run it, but treat the observation as ground truth in the meantime and do NOT keep chasing the interpolation tier. The cause is in the emitter / transform itself: something recomputes the banner's position per frame in a way that is not stable, and it is worst when the camera moves — so the instability is most likely in the WORLD->SCREEN step, not in the banner's own world position. POSSIBLE SHARED MECHANISM WITH #73: that card is a screen-space popup 'anchored to Tomba' whose anchor is wrong in widescreen. Both #71 and #73 are screen-space elements positioned from a world anchor. If they share that anchoring path, one defect explains both, and the widescreen commits of 2026-07-28..31 touched exactly that arithmetic. Check whether they share code before treating them as two bugs.

**2026-08-04:** 2026-08-04 SANDBOX BUILT + #71 MEASURED FOR THE FIRST TIME. Repro is now one command.

THE TOOL (the user's actual ask). tools/sandbox.py — a CLIENT of the real game over the debug
server, deliberately NOT a second binary (a separate exe would carry its own renderer/camera/object
graph and drift from the shipping path). Spawning goes through the GAME'S OWN spawner: the debug
server's existing `call` invokes a guest function on the live CPU, so the recipe for this banner is
literally CubeTextLedger::spawnPopup — `call 80040AA4 38 0` -> v0=800FB218, the node kanban #64
identified. No new engine mechanism was added to spawn anything. Framework side gained exactly two
commands, both forwarding to functions the REPL already called: `preseq` and `tp` (they were
REPL-only, and the REPL blocks the frame loop, so the ONLY headless instrument that can see
interpolated frames was unreachable from a live session). Docs: docs/driving-the-game.md.
Repro: PSXPORT_SETTINGS=... tools/sandbox.py scenarios/banner-camera-pan.txt --port 5971

WHAT THE VIBRATION ACTUALLY IS, MEASURED. Instrument: `debug preseqobj` logs one line per drawn
RqItem per PRESENT with key=<node address>, so the banner's own prims can be isolated (196/present)
and differenced. Window = the last 18 presents, after the rise-in animation has settled (min/max x
stable at 41/271), camera panning (holding Right).
  fps60 ON, per-present X shift of the banner's prim distribution (order-free, sorted-vector):
    -1.22 +1.55 -1.58 +1.24 -1.24 +1.49 -1.38 +0.79 -1.82 +2.72 -1.88 +1.54 -1.44
    10 of 12 usable comparisons ALTERNATE SIGN. mean |dX| = 1.53 px. NET drift = -1.23 px.
  i.e. the banner travels essentially NOWHERE while shaking +/-1.5 px every present. That is the
  user's "vibrating", as a number.
  Control, SAME RUN, same instrument: the other text-label node 800FB218, still panning, scores
  2/16 alternations and -39.4 px of net travel. So the discriminator was run against both classes
  and separates smooth motion from oscillation rather than flagging everything.
  AXIS ASYMMETRY: X 1.53 px/present vs Y 0.43 px/present. X is the axis the follow camera pans
  along. Consistent with the WORLD->SCREEN step being unstable rather than the banner's own world
  position (a jittering world position would jitter with the camera still too) — NOT yet proven.

THE fps60 A/B IS NOT CONCLUSIVE — do not record it as one. The fps60=0 leg measured 0.32 px mean and
3/11 alternations, which LOOKS clean, but that measurement is compromised and I am not standing
behind it: with fps60 off the banner emits 392-393 prims per present instead of 196 (it appears
TWICE), so 6 of 17 consecutive comparisons had mismatched counts and were unusable, and the doubled
set averages a shift away. USER, observing the running game: "I think it still happens without
60fps." Treat the user's observation as operative and the lerp tier as NOT the explanation. The
392-vs-196 double emission with fps60 off is itself unexplained and previously unrecorded — it may
be a real defect or a preseq present-index artifact; it needs its own look.

#73 DOES NOT SHARE THIS ANCHORING PATH — checked, negative, so #71's scope is NOT widened by it.
The banner is a WORLD-PROJECTED element: textLabelEmit + subPartCapture push Render::WqRec, factored
against the scene camera by wq_factor_world, drawn by Render::billboardsRender. ScorePopup
(game/render/score_popup.cpp) is a 2D UI element: it collects UiGroupArgs and draws via
Render::emitUiFt4 / emitUiSprites into RQ_OVERLAY, and contains no 320/wide-engine width arithmetic
of its own (grep for 320|wide_engine|projCompose|WqRec|billboard in score_popup.cpp: 0 hits). Two
different mechanisms; a fix to one does not reach the other.

NOT FIXED, and deliberately so. The mechanism is not yet named, and per the no-hacks rule an
unexplained nudge that makes the numbers settle would be worse than leaving it broken. NEXT, and the
sandbox makes each of these cheap: (a) run the same scenario with the camera HELD STILL (tp) and
re-measure — if the oscillation vanishes it is in the world->screen step, if it persists it is in
the banner's own transform; (b) log the banner's world anchor (WqRec objT) alongside its screen x in
the same frame and see which one oscillates; (c) look for a +/-1-unit quantisation in that step
(fixed-point truncation, an intermediate rounded to integer pixels, a divide whose rounding
direction flips as the camera crosses a boundary) — a quantity oscillating by exactly one unit is
the signature; (d) explain the 392-vs-196 double emission.

**2026-08-04:** 2026-08-04 ROOT CAUSE NAMED, with the decisive measurement. NOT fixed — the proper fix is a
contract change with real blast radius; it is named below rather than patched around.

THE MECHANISM: the cube-text node's transform is CAMERA-FREE, but the display-pass capture factors
it as though a camera were composed into it, then re-applies the camera to draw. That round trip is
the defect.
  - textLabelEmit (text_label.cpp:143) and subPartCapture (subpart_capture.cpp:84) both call
    wq_factor_world (render_internal.h:84-101), whose contract is "CR = cam . obj": it computes
    objT = camT_transpose * (tr - camT), reading the scratchpad scene camera at 0x1F8000F8.
  - PROVEN by direct measurement that that contract does NOT hold for this node class. Probed the
    live glyph matrices at cmd+0x18 (+0x14 = the libgte MATRIX translation) over 14 stepped frames
    while holding Right, camera eye X travelling 3289 -> 3672:
        cmd[0] tr = [-114, -62..-67, 358]      cmd[5] tr = [-54, -63..-67, 358]
    Z is EXACTLY 358 on every frame for both; X is EXACTLY -114 / -54 on every frame. The camera
    moved 383 units and the transform did not move at all. The only motion is a few units of Y,
    which is the per-glyph idle bob (leaf_8003A3E8, a sine of amplitude ~3 with a random per-glyph
    phase). The 12-unit X spacing between glyph 0 and glyph 5 matches the row layout
    X = (glyphIndex - len/2) * 12 (leaf_8003A1E4), and Z=358 sits at the projection plane
    H=350 (0x801003F8). This is a VIEW-LOCKED billboard: constant position in front of the camera.
  - So wq_factor_world is fed a matrix with no camera in it and synthesises a fake WORLD position
    that rides the camera; billboardsRender (perobj_billboard.cpp:757-812) then re-applies the
    camera to project it. camT_transpose-then-cam is identity only in exact arithmetic. The camera
    is s16 fixed point (1/4096, CR-packed) read into float, the factor subtracts an s32 camT, the
    re-apply multiplies by the camera again. Every step quantises, and the quantisation error is a
    FUNCTION OF THE CAMERA — so it changes every frame the camera moves, and is constant when the
    camera is still.

THAT PREDICTION WAS TESTED AGAINST BOTH CLASSES, and it holds:
  CAMERA STILL   (fps60 on): dX per present = -0.15 +0.03 +1.46 +0.03 -0.28 then 0.00 x10
                             mean |dX| = 0.13 px, 2/14 alternations. DEAD STILL, exactly 0.00 for
                             ten consecutive presents.
  CAMERA PANNING (fps60 on): dX = -1.22 +1.55 -1.58 +1.24 -1.24 +1.49 -1.38 +0.79 -1.82 +2.72
                             -1.88 +1.54 -1.44
                             mean |dX| = 1.53 px, 12/12 alternations, NET drift -1.23 px.
  Same object, same instrument, same settled window. The vibration is entirely camera-motion-gated.

This explains every reported property, including the ones that killed the earlier lead: it vibrates
rather than drifts (the error is per-frame, not systematic); it is worst under camera motion (the
error is a function of the camera); it looks fine frozen (a static camera gives a constant sub-pixel
offset); it is X-dominant, 1.53 px vs 0.43 px in Y (the pan is in X); and it does NOT require fps60,
because the factor/re-apply round trip runs on every real frame — the lerp tier only adds a second
camera to the same round trip. The user's "I think it still happens without 60fps" is consistent
with this mechanism, and #64's fix is unrelated to it (that one equalised glyph-vs-plank, and both
halves go through this same wq_factor_world, so it could not have helped).

THE PROPER FIX, not applied: a view-locked node class must not be round-tripped through world space
at all. WqRec needs to carry the space its transform is in, and billboardsRender must skip the
camera re-apply for a view-space record, so the guest's own view-space position is used verbatim.
That is a change to a SHARED contract — WqRec is produced by text_label.cpp, subpart_capture.cpp,
render_walk.cpp (perObjFlushPreComposed), quad_rtpt_submit.cpp, widescreen_margin_quad.cpp and
submit.cpp, and consumed by billboardsRender including its fps60 lerp — so it needs its own SBS +
pixel A/B and an interaction check with #64. Shipping it half-verified, or nudging the arithmetic
until the numbers settle, would be the bandaid this rule exists to prevent. Note render_walk.cpp:139
applies the SAME factoring to every pre-composed-matrix node, so any other view-locked node class is
affected identically — worth enumerating before designing the fix.

REPRO, one command:
  PSXPORT_SETTINGS=scratch/fps60_on.ini tools/sandbox.py scenarios/banner-camera-pan.txt --port 5971
  (still-camera control: same file with `press right` removed)
Measurement: `debug preseqobj` + `preseq`, group by key=<node addr>, diff sorted screen-x vectors.

**2026-08-04:** USER 2026-08-04, and this REJECTS the fix proposed above: 'If these are properly ported then the vibrations shouldn't happen because we'd know if it was vibrating because there would be code that makes it vibrate.' Correct, and decisive. Nothing in the game vibrates this banner — no jitter routine, no noise term. So 100% of the motion is manufactured by the PORT, and any fix that REDUCES it is compensating for a mechanism that should not exist. DO NOT implement 'WqRec carries its transform space, skip the camera re-apply': it keeps the tap, keeps the GTE read, keeps the s16 quantisation, and merely shrinks the residue. REAL CAUSE, one level up: this effect is not ported, it is TAPPED. quad_rtpt_submit.cpp:241-245 reads the GTE CONTROL REGISTERS (gte_read_ctrl(5+i), (int16_t) matrix words) after the substrate ran, then wq_factor_world un-composes the camera out of that already-quantised matrix and the renderer re-applies the native camera. camT-then-cam is identity only in exact arithmetic, so the residue is a FUNCTION OF THE CAMERA and changes every frame it moves. CLAUDE.md bans exactly this ('never reproduce GTE output'; 'a tap is a SCAFFOLD, not the destination', USER 2026-07-23). REAL FIX: retire the tap into a native producer that reads the effect's own world state and projects with the native camera — no GTE read, no factoring, no round-trip. Then vibration is structurally impossible rather than merely small. Note the standing gate that forces this anyway: a tap CANNOT lerp, so any effect that must interpolate at 60fps has to be a real port by construction. SCOPE: wq_factor_world has callers in text_label.cpp:143 and quad_rtpt_submit.cpp:245, and render_walk.cpp:139 applies the same factoring to EVERY pre-composed-matrix node — so every view-locked class through that path shares this defect.

**2026-08-04:** 2026-08-04 FIXED, and the fix is the RETIREMENT OF THE TAP, not a correction to it.

WHAT WAS WRONG. The banner had no native producer. Its picture came from reading back the per-glyph
libgte MATRIX the guest had already composed (cmd+0x18 == sub+0x18 — textLabelEmit's cmd and
subPartWalk's sub are the SAME pointer, node+0xC0[i]), un-composing the scene camera out of it
(wq_factor_world) and letting billboardsRender re-apply the camera. cam^T-then-cam is identity only
in exact arithmetic; the camera is s16 fixed point; so the residue was a function of the camera and
moved every frame the camera moved. That is 100% of the vibration. Nothing in the game produced it.

WHAT WAS DONE. Break-first, per the standing rule. Every tap was DELETED and the layer was confirmed
honestly absent (node 800FB218: 196 prims/present -> 0, and gone from the screenshot) BEFORE anything
was rebuilt. wq_factor_world and wq_read_matrix are gone from render_internal.h; subpart_capture.cpp
is deleted; Render::perObjFlushPreComposed is deleted; the WqRec record type, its two buffers and the
fps60 swap that rotated them are gone, because their only purpose was carrying a factored transform
back to a camera re-apply. Then the real producer: game/render/cube_text_banner.cpp.

THE PRODUCER. The cube-text node is a VIEW-SPACE billboard — its transform IS the modelview. RE
(19 functions on the write path audited): the matrix has exactly one writer, NodeXform::propagateRotmat
(already native), and NOTHING on that path reads the scene camera or a GTE control register. So
CubeTextBanner recomputes propagateRotmat's own math from its own inputs — rec.R = node.R *
rotmat(rec+0x08), rec.T = node.R*(rec+0,+2,+4) + (node+0x2E,+0x32,+0x36) — and projects with
ofx/ofy/H and NO camera at all. Vibration is not reduced, it is structurally impossible. It draws the
glyph AND the record's plank from that one transform, so #64's glyph-vs-plank drift is also
structurally impossible.

THE NUMBERS (tools/preseqobj_check.py --node, now a scripted gate; same object, same window, same
instrument as the defect measurement):
  camera PANNING  before: mean |dX| 1.48 px, 12/12 sign alternations, net -1.35 px
                  after:  mean |dX| 0.15 px,  0/16 sign alternations, net +2.60 px
  camera STILL    after:  mean |dX| 0.00 px exactly, 0/16 alternations
  The 0.15 px/present that remains is MONOTONE and is the banner's own bounce-out animation: the
  measured dY sequence -14.9 -13.0 -11.1 -9.5 -7.2 -5.6 -3.7 px is exactly the guest integrating
  rec+0x12 from -256 at +32/frame. Every pixel of motion now traces to code in the game.
  Instrument validated on the pre-change binary (reproduced the original 1.53 px / 12-12 / -1.23) and
  against a node key that does not exist (reports NOTHING WAS MEASURED and fails, rather than 0.00).

NOT A GUEST-STATE CHANGE: guest RAM (2 MB) and scratchpad are byte-identical to the pre-change binary
over a spawn + 40-frame run.

HERMETIC GATE: PSXPORT_SELFTEST=cubetext asserts the producer's screen output is byte-identical under
two very different cameras, with a NEGATIVE CONTROL proving the comparator sees an 81.2 px change for
a camera-composed projection of the same points, and a prim-count assertion so a producer that drew
nothing cannot pass. It went red first and caught a real bug (the string NUL was ending the record
loop, dropping every trailing plank).

REPRO / gate commands:
  PSXPORT_SETTINGS=scratch/fps60_on.ini tools/sandbox.py scenarios/banner-settled-pan.txt --port 5971
  PSXPORT_SETTINGS=scratch/fps60_on.ini tools/sandbox.py scenarios/banner-settled-still.txt --port 5972
  python3 tools/preseqobj_check.py scratch/logs/sandbox_5971.log --node 800FB218
