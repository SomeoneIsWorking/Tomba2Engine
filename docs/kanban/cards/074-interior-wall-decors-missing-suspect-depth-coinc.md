---
id: 74
title: Interior wall decors missing (suspect depth / coincident-face ordering)
status: done
labels: [render]
created: 2026-08-04
updated: 2026-08-20
evidence: docs/reference/issues/issue74_house_pc_render_decor_missing.png,    docs/reference/issues/issue74_house_psx_render_decor_present.png
---

**2026-08-04:** USER-REPORTED 2026-08-04: interior wall decors missing, user suspects depth. CRITICAL CONTEXT: this is the same mechanism as kanban #29 (hut-interior wall decals — coincident faces in one OT bucket resolved by submission order), which is handled by RenderQueue::resolveKeyOrder. I rewrote resolveKeyOrder TODAY (psxport ffba3eaa) from pairwise enumeration to witness search. That change was gated by 5 tests pinning the snap set against a brute-force oracle, passing on BOTH sides — so it should be behaviour-preserving. BUT: equivalence to the PREVIOUS behaviour preserves a defect if the decals were already broken before it. Two distinct questions, answer both: (a) were the decors already missing BEFORE ffba3eaa? (b) does ffba3eaa change them at all? Check (a) first by testing a pre-ffba3eaa build; if they were already missing, ffba3eaa is exonerated and the cause is in the 07-28..07-31 window.

**2026-08-05:** TRIAGED 2026-08-05 — NOT ROOT-CAUSED. Honest status: I answered the card's question (b) and did NOT answer (a). (b) ffba3eaa IS EXONERATED, by reading rather than by bisect (the user has since ruled bisect out). The card's worry was that the witness-search rewrite might skip a face that another face still needs as EVIDENCE. It does not: the outer loop skips an already-decided face ('if (snap[a]) continue') but the INNER witness loop runs the full group_start..group_end with only 'b == a' excluded — there is no 'if (snap[b]) continue'. The skip is on the QUESTION, never on the EVIDENCE. rq_faces_in_contest is symmetric (same-key branch is a symmetric coincidence test; different-key branch canonicalises to near/far by sort_key before testing), so new snap[a] == old snap[a] for every face. (a) STILL OPEN: whether the decors were already missing before ffba3eaa. NOT tested — it needs a pre-ffba3eaa build, and CLAUDE.md forbids a temporary-revert A/B in this shared checkout, so it needs its own clone. NEW CONFOUND that must be controlled for first: Tomba2Engine eddd280 ('Retire the render taps'), landed 2026-08-04 23:54 — i.e. about 90 minutes AFTER this card was filed at 22:17. It cannot be the cause of what the user reported, but it DELIBERATELY deleted the pc_render picture for quad_rtpt_submit.cpp and widescreen_margin_quad.cpp (the a00 flame/rope emitter, case-188 particles, B704 beams, drum/windmill margin props). So any measurement taken on the current binary sees those layers absent for a reason that is NOT this bug. Decide FIRST which producer the hut decors actually use: #29's evidence points at drawWorldQuad + keyed faces on node 800FD850 (untouched by eddd280), candidate A points at submitQuad (killed by it). One run settles it: PSXPORT_DEBUG=quadrtpt,preseqobj,keyord with PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-door-freeze.pad, run 1200, then grep for key=800FD850 and for any interior node under quadrtpt. Repro recipe confirmed from replays/README.md + docs/hut-render-bug.md: 'warp' CANNOT reach this hut (it is a sub-scene swap inside the village, scene-active stays 2, only sm[0x4c] goes 2->3).

**2026-08-05:** CONFOUND CONTROLLED + ffba3eaa EXONERATED A SECOND, EMPIRICAL WAY. One run, PSXPORT_DEBUG=quadrtpt,keyord with PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-door-freeze.pad, run 1200, headless. (1) THE eddd280 CONFOUND DOES NOT APPLY TO THIS BUG: QuadRtptSubmit::submitQuad emitted ZERO lines for the whole run (the only 'quadrtpt' match in the log is the [cfg] banner echoing the env var — beware, that echo is what makes a naive grep -c report 1 and look like a hit). The layers eddd280 deliberately deleted are therefore NOT the hut decors' producer, so today's binary can be measured for this card without that confound. (2) ffba3eaa IS IRRELEVANT HERE, measured rather than reasoned: across 10176 keyord lines, the number of resolveKeyOrder invocations reporting a NONZERO keyed-face count is ZERO (distribution is '0 keyed faces of N queued prims' for N up to 668). resolveKeyOrder never contests anything in this scene, so a rewrite of how it contests cannot change this picture. That is an independent confirmation of the 2026-08-05 exoneration-by-reading. (3) THE DECOR GEOMETRY IS SUBMITTED: node 800FD850 / geomblk 8018A03C, declaring gt3=154 gt4=382, submitted 2428 times. (4) AND AT THIS VIEWPOINT IT RENDERS: screenshot scratch/screenshots/k72/hut_f1200.png shows the interior WITH its wall decor — shelves, hanging lures, fishing rod, baskets, corn, barrels. So the card's symptom does NOT reproduce at this viewpoint on this replay. NOT YET RULED OUT: the SECOND interior viewpoint (replays/README.md: 'press left' + 'run 80' scrolls the camera to it) and other interiors — that is where to look next. CAUTION, MIS-CALIBRATED INSTRUMENT FOUND (game/render/submit.cpp:400-418): kImplausiblePolyCount=256 makes gt3gt4 log 'geomblk ... — not a geometry block' for node 800FD850's PERFECTLY LEGITIMATE 382-gt4 block, 2428 times. The real garbage case the threshold was calibrated against read gt3=63740 gt4=64760 — over 150x larger. The check is DIAGNOSTIC-ONLY (nothing is skipped; submitPolyGt3/4Native run regardless) so it is NOT the cause of any missing layer, but it cries wolf on real geometry and will send the next agent down a false trail. Raise the threshold toward the actual garbage signature, or gate on a structural test rather than a magnitude guess.

**2026-08-05:** REPRODUCED 2026-08-05 — first time, and NOT where the card was looking. It is NOT the hut and NOT a depth/ordering bug.

WHERE IT REPRODUCES: 'House on the Point' interior, replays/bugs/house-on-the-point.pad at frame 3000 (REPL 'run 3000', PSXPORT_VK_HEADLESS=1). Evidence attached: issue74_house_psx_render_decor_present.png vs issue74_house_pc_render_decor_missing.png.

THE DISCRIMINATOR, run properly per docs/gfx-debug.md: two runs from BOOT over the same bit-deterministic pad replay, same frame index, differing ONLY in PSXPORT_RENDER_PSX (set at boot, never toggled mid-scene — the kanban #41 trap). In-band leg proof taken from the [cfg] banner echo, not from the pixels being measured. psx_render shows a dense row of hanging wall decorations along the left wall, blue/teal hanging items, a mounted trophy on the back wall and a green item by the window; pc_render shows bare planks in every one of those places. Same game state both legs — only the renderer differs.

THE HUT IS EXONERATED AS A REPRO SITE, both viewpoints. replays/scene-transitions/hut-entry-door-freeze.pad, run 1200 (view 1) and +'press left'+'run 80' (view 2, the one the previous note left open): the fisherman's-hut interior renders WITH its full decor in pc_render at both — shelves, hanging lures, fishing rod, baskets, corn, barrels. So the 2026-08-05 'does not reproduce at this viewpoint' result now covers the whole replay, and the hut should not be used to chase this again.

WHAT IT ACTUALLY IS: a layer with NO NATIVE PRODUCER, not a depth or coincident-face bug. The card's title suspects depth; that reading is wrong. PSXPORT_DEBUG=rendernative at the repro frame: 'scene: 3 live, 2 3D-mesh nodes, 17 geomblk objects (H=350 ofx=160)' then 'drew 17 objects, 615 prims'. The decor is not in the entity list the native scene pass walks at all, so no ordering decision could have dropped it — nothing ever enumerated it. psx_render gets it because it walks the guest OT and draws whatever the guest emitted. This puts #74 in the same family as the four already-tracked absent layers, NOT with #29.

CANDIDATE PRODUCER, not yet confirmed: PSXPORT_DEBUG=otattr + REPL 'otattr' at the frame reports GTE RTPS/RTPT per-node counts of node=0x800FD748 count=636, node=0x800FC9E0 count=185, node=0x800E7E80 count=310, and fn=0x8002AB5C node=0x800EDB80 count=2. 0x800FD748 (636 transforms in ONE frame) is the strongest candidate for the decor emitter — note it is adjacent to, but NOT the same as, the hut decor node 0x800FD850 from #29. CAVEAT ON THAT NUMBER: fn=0 on three of the four rows, i.e. otattr could not attribute the emitting function — its class comment states it is blind to fully-native (non-dispatched) draw paths, so 'fn=0' here means 'not attributed', NOT 'no function'. Do not read the node numbers as a producer identification until the emitter is confirmed by another route (Ghidra on the writer of 0x800FD748, or otattr watch/who on it).

SECOND MISSING LAYER SEEN IN THE SAME PAIR, filed here so it is not lost: psx_render draws a 'Use UP + O to talk' dialog prompt at this frame and pc_render draws nothing there. Same state, same frame. Not investigated.

**2026-08-05:** PRODUCER HUNT, iteration 2 — two hypotheses KILLED, one sharp discrepancy left.

THE NODE, read live at the repro frame (REPL 'r'/'rw'/'ents' at f3000 of house-on-the-point.pad):
  800FD748  t=03 ri=00 model=0000 h=8012C910 pos=(14596,-2462,809) rf=1 cmds=2 gb0=8018F9AC  [PSX]
It is in LIST 1, its live byte (+0x01) is 1, it has 2 render commands and a valid geomblk, and its
behaviour handler 0x8012C910 has NO native owner — codemap.py --addr reports that against a stated
denominator (1035 indexed natives / 472 install sites / 11 PlatformHle / 48 port-map steps) and lists
what it is blind to, so 'no owner' here is a real negative and not an empty search.

KILLED HYPOTHESIS 1 — 'collect() skips it because ri != 0x0F'. WRONG, and the source of the error was a
STALE COMMENT in scene_build.cpp itself: the banner said '0x0F = 3D mesh (we draw these)', describing an
allow-list. The actual code (line ~92) is a DENY-list: 'if ((ri >= 0x10 && ri <= 0x14) || ri == 0x20)
continue' — everything else draws, and ri=0x00 is the field's normal static-prop value. So ri=0x00 is NOT
why this node is missing. Comment fixed in the same commit so the next reader does not lose the same hour.

KILLED HYPOTHESIS 2 — 'the two walkers read different list heads'. WRONG, checked rather than asserted:
scene_build.cpp:78 and repl.cpp:134 both use the identical triple { 0x800FB168, 0x800F2624, 0x800F2738 }
and both dereference it. Same lists, same order.

THE REAL DISCREPANCY, and the next thing to chase: over those SAME lists at the SAME frame, 'ents'
enumerates ~130 nodes while PSXPORT_DEBUG=rendernative reports 'scene: 3 live, 2 3D-mesh nodes, 17
geomblk objects'. Both walk head -> +0x24 chains. The two loops differ in exactly two ways and one of
them must explain a 130-vs-3 gap:
  (a) the LIVE TEST. collect() does 'if (mem_r8(n + 1) == 0) continue' and counts what survives; ents
      applies no live filter at all. But 800FD748's +0x01 reads 1, so this node should have survived —
      unless the byte differs AT THE MOMENT collect() RUNS. collect() runs inside the render pass; 'ents'
      runs from the REPL after the frame completed. If the lists are (re)built or the live flags set
      later in the frame than the render pass, collect() legitimately sees an almost-empty world and the
      bug is ORDERING WITHIN THE FRAME, not enumeration. THAT IS THE LEADING HYPOTHESIS and it is
      directly testable: log n/+0x01/ri per visited node from inside collect() and compare against ents
      at the same frame.
  (b) the TERMINATION TEST. collect() walks while 'n >= 0x80000000 && n < 0x80200000 && g < 1024'; ents
      walks while 'n && g < 400'. A next-pointer that is non-zero but outside [0x80000000,0x80200000)
      stops collect() DEAD while ents keeps going — so a single out-of-range link early in list 0 would
      truncate collect() to a handful of nodes and leave ents' count untouched. Also fits 130-vs-3.
Distinguish (a) from (b) with one instrumented run; do not guess between them.

NOT YET DONE: 0x8012C910 has not been decompiled. Do (a)/(b) first — if collect() is being truncated or
run too early, the emitter's identity does not matter yet.

**2026-08-20 CORRECTION TO THE HUT EXONERATION:** the current deterministic HUT replay DOES reproduce
the user's live missing central hanging decoration. The old screenshot-based conclusion trusted a
different visual target and the packet-present probe did not establish the final painter. Same-scene
queue/presentation evidence now shows both HUT materials reach Native, but same-bucket order is reversed
relative to the GTE OT walk because AddPrim head insertion was not modeled; fixed under #57. This does
not close this card's separate House on the Point producer gap, which remains the next work here.

**2026-08-21 CURRENT-TREE RECHECK, NOT YET A CLOSURE:** a faithful Native A pane at the historical
f3000 viewpoint now visibly contains the left-wall hangings, blue/teal items, trophy, and talk prompt
that the 2026-08-05 pc-render capture lacked. That contradicts this card's old “no native producer”
diagnosis: the cited `NativeScenePass::collect` census is a debug-only `rendernative` pass, not the
shipping `Render::fieldObjectsRender` owner, so its 3-live/17-object denominator could never prove
that the shipping queue omitted the decor. The paired software-B pane is NOT evidence: on
`house-on-the-point.pad`, B is still outside the house while A is inside and RAM/scratchpad differ
throughout the survey run. This is the known I053 same-state limitation in a gross rather than
one-tick form. Two standalone faithful Native attempts then stopped in framework Vulkan target
allocation under host contention before reaching the replay, so the remaining gate is a clean
standalone f3000 capture plus queue/key attribution. Do not close this as the systemic OT-LIFO fix
until that attribution proves it; do not revive the debug-collector producer theory.

**2026-08-21 FIXED — CURRENT SHIPPING QUEUE ATTRIBUTION CLOSES THE CARD:** the remaining
“standalone faithful” request above was based on a retired interface. The product now has one
standalone execution policy; faithful Native is intentionally the SBS A leg. A fresh bounded run on
clean psxport `2b5ef7b5`, `house-on-the-point.pad`, `PSXPORT_SBS_MODE=oracle`, and
`PSXPORT_DEBUG=keyord` captured the historical f3000 viewpoint. Pane A visibly contains every target
from the old missing screenshot: the dense left-wall hangings, blue/teal items, mounted trophy, and
the talk prompt. Pane B was still outside and was not used as picture evidence.

The shipping queue evidence names the old candidate rather than merely showing a good screenshot:
Native A queued 467/467 world faces with keys; node `0x800FD748` contributes faces from sequence 76
through 343; 121 faces were snapped to authored order and 96 faces used same-bucket OT-LIFO ties.
Thus the old “no native producer” conclusion was false twice over: `NativeScenePass::collect` is a
debug-only census, and the alleged missing node is present in the shipping queue at the exact frame.
The failure was final same-bucket presentation order, the same systemic AddPrim head-insertion rule
fixed under #57—not a House-specific emitter gap. C052 records the falsifiable result. Evidence:
`scratch/logs/house_oracle_keyord.log` and
`scratch/screenshots/house_oracle_keyord_f3000_A.ppm`.
