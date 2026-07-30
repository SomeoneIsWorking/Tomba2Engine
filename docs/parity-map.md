# SBS parity map — is each ported unit byte-exact to recomp_path? (managed by tools/parity.py)

Durable ledger for Job #1 (byte-exact pc_faithful). One `## ` block per ported unit.
`tools/parity.py` = summary · `tools/parity.py <words>` = search · `tools/parity.py check` = gate.

**Status:** 66 verified · 9 partial · 12 untested · 7 n/a

## ActorTomba::actionHandler800531DC (FUN_800531DC)
- **status:** verified
- **frames:** 27150
- **gate:** port_check PASS + MIRROR_VERIFY OK + 0 sbs-div; combat + watch-cut f27150
- **evidence:** 58809b1c

## ActorTomba::actionHandler800588BC (FUN_800588BC)
- **status:** verified
- **frames:** 21390
- **gate:** port_check PASS + MIRROR_VERIFY OK + 0 sbs-div; combat + watch-cut f21390
- **evidence:** 71b5d764

## ActorTomba::actionHandler8005ACC8 (FUN_8005ACC8)
- **status:** verified
- **frames:** 20040
- **gate:** port_check PASS + MIRROR_VERIFY OK (469 hits) + 0 sbs-div; combat f5700 + watch-cut f20040
- **evidence:** 2a94898c

## ActorTomba::actionHandler8005AEE4 (FUN_8005AEE4)
- **status:** verified
- **frames:** 21390
- **gate:** port_check PASS + MIRROR_VERIFY OK + 0 sbs-div; combat + watch-cut f21390
- **evidence:** 71b5d764

## ActorTomba::enterOuterState0 (FUN_80058648)
- **status:** verified
- **frames:** 19740
- **gate:** MIRROR_VERIFY pass#1 OK + combat clean-exit 0-diff f4000 + watch-cut 0-diff f19740
- **evidence:** c47d3690

## ActorTomba::matrixComposeAttached (FUN_800597AC)
- **status:** verified
- **frames:** 18900
- **gate:** 11713 MIRROR_VERIFY passes + 0 sbs-div/6000 combat frames; watch-cut 0-diff f18900
- **evidence:** 537dac98

## ActorTomba::mode0ActionGate (FUN_8005A910)
- **status:** verified
- **frames:** 20580
- **gate:** MIRROR_VERIFY OK all invocations + 0 sbs-div; combat 0-diff + watch-cut f20580
- **evidence:** 0bb8cb9d

## ActorTomba::mode0WalkHandler (FUN_8005A970)
- **status:** verified
- **frames:** 20040
- **gate:** port_check PASS + MIRROR_VERIFY OK + 0 sbs-div; combat 0-diff f5400 + watch-cut f20040
- **evidence:** d4ace056

## chain-opn-hook-18c820
- **scope:** 0x8018C820 OPN-overlay assembly hook (12th chain leaf)
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=31 oracle=31 balanced; port_check PASS. rec_dispatch-only interception is COMPLETE for this address, not partial — it has exactly one caller in the image and that caller uses rec_dispatch (verified by grep of generated/).

## chain-perchild-xform-12e8a8
- **scope:** 0x8012E8A8 per-child transform propagate
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... seesaw-weight.pad
- **evidence:** 50/50 A/B-identical, zero divergence; ovhit native=1595 oracle=1595 balanced; port_check PASS.

## chain-posttick-146348
- **scope:** 0x80146348 assembly post-tick
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... seesaw-weight.pad
- **evidence:** 50/50 A/B-identical, zero divergence; ovhit native=1394 oracle=1394 balanced; port_check PASS.

## chain-state0-init-12ed84
- **scope:** 0x8012ED84 STATE 0 init
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... seesaw-weight.pad
- **evidence:** 50/50 A/B-identical, zero divergence; ovhit native=24 oracle=24 balanced; port_check PASS.

## chain-substate1-12f5b4
- **scope:** 0x8012F5B4 sub-state 1 tick
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... seesaw-weight.pad
- **evidence:** 50/50 A/B-identical, zero divergence; ovhit native=17 oracle=17 balanced; port_check PASS.

## collision-classify-1f40c
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** 6a80b0c (50/50 identical, ovhit 1796/1796)

## collision-landing-2423c
- **scope:** actor-vs-object vertical landing snap FUN_8002423C
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** RE-GATED 2026-07-30 after the readability pass: 50/50 A/B-identical checkpoints, zero divergence; ovhit 0x8002423C native=1949 oracle=1949 (balanced). port_check.py PASS.
- **owner:** game/world/collision_resolve.cpp CollisionResolve::landOnObjectTop

## collision-resolve-23d48
- **scope:** actor-vs-object cylinder collision resolve FUN_80023D48
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=7397 oracle=7397 (balanced). port_check.py PASS. RE-GATED 2026-07-30 after the readability pass (gotos -> if/else, register chains -> named locals/GuestReg, GuestFrame + guest_call): same 50/50 and same 7397/7397. The dead-stack hazard did not materialise because the port still calls the leaves through their func_XXXX wrappers, so each callee writes its own guest frame.
- **owner:** game/world/collision_resolve.cpp CollisionResolve::cylinderResolve

## collision-terrain-snap-4766c
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** 6a80b0c (50/50 identical, ovhit 3054/3054)

## contact-stamp-111304
- **scope:** 0x80111304 contact stamp producer
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=1151 oracle=1151 balanced; port_check PASS.

## contact-weight-apply-1308e0
- **scope:** 0x801308E0 contact-index to weight consumer
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=16680 oracle=16680 balanced; port_check PASS.

## Core::guestMemset (FUN_8009A420)
- **status:** verified
- **frames:** 18120
- **gate:** same wave gate; ovhit 2247/2247; §9 n<=0 return-0 fix
- **evidence:** 7db286a8

## cull-arm-eulerz-swing-133700
- **scope:** 0x80133700 arm the one-shot decaying Euler-Z swing
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=8364 oracle=8364 balanced; port_check PASS.

## cull-reverse-swing-1332c4
- **scope:** 0x801332C4 reverse swing on crossing + cache peer swing bit
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=8364 oracle=8364 balanced; port_check PASS.

## cull-substate-zero-132954
- **scope:** 0x80132954 sub-state-zero tick
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=8364 oracle=8364 balanced; port_check PASS.

## cull-swing-phase-132a88
- **scope:** 0x80132A88 phase-advancing Euler-Z swing sibling
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=8364 oracle=8364 balanced; port_check PASS.

## cull-tick-eulerz-swing-133550
- **scope:** 0x80133550 child Euler-Z decaying swing tick
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=8364 oracle=8364 balanced.

## DEMO/title front-end (whole-scene)
- **scope:** stage 0x801062E4 boot->title menu (s48 handoff + menu hold + cursor)
- **status:** verified
- **frames:** 10890
- **gate:** PSXPORT_SBS_MODE=gameplay ... (both cores psx_render; isolates gameplay logic from render)
- **evidence:** f2d1b8af 2026-07-15
- **owner:** game/scene/demo.cpp
- **notes:** gameplay mode used because pc_render's fail-fast would abort core A on an unbuilt scene; render is guest-RAM-neutral so this is a valid logic gate

## Demo::s2SubMachine
- **scope:** 0x8010696C (title New/Load cursor sub-machine)
- **status:** verified
- **frames:** 10890
- **gate:** PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_SBS_MODE=gameplay ./scratch/bin/tomba2_port  (drive title menu; expect A/B identical)
- **evidence:** f2d1b8af 2026-07-15 — 10890 frames 0-diff incl. cursor input; dispatch confirms override fires
- **owner:** game/scene/demo.cpp (Demo::s2SubMachine)
- **notes:** byte-exact frame 32 per abi_extract; twin of s3SubMachine

## Dialog glyph tap FUN_8007CC00 (Panel::pushDialogGlyphs)
- **status:** verified
- **frames:** 19590
- **gate:** SBS-full combat f5460 + watch-cut f19590 0-diff; hut replay bubble identical via tap (panelq box=800EEA60 count=17)
- **evidence:** 916ddfc0

## drive-accel-select-130788
- **scope:** 0x80130788 drive-axis acceleration selector
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=11120 oracle=11120 balanced; port_check PASS. RE claims self-verified against ov_a00_shard_0.c:17191-17256 because batch-5's verify agents died on server errors.

## driven-pair-offset-1314b4
- **scope:** 0x801314B4 re-place driven pair from tilt angle
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=65 oracle=65 balanced; port_check PASS.

## engine-alloc-record-selector-8913c
- **scope:** 0x8008913C record[0]/record[1] selector over the 0x80102500 array
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=1504 oracle=1504 balanced.

## engine-pad-fence-tail-5229c
- **scope:** 0x8005229C padEdgeFence tail 4-phase state machine
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=1500 oracle=1500 balanced.

## Engine::padEdgeFence (FUN_800788AC)
- **status:** verified
- **frames:** 18120
- **gate:** combat clean-exit 0-diff f4500 + ovhit 4500/4500; watch-cut 0-diff f18120
- **evidence:** 7db286a8

## Engine::walkStart (early-exit frame mirror)
- **status:** verified
- **frames:** 23850
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_SBS_WATCH_CUT=1 (0 sbs-div through f23850) + AUTONAV=combat (0 through f7290)
- **evidence:** 1ff117a1

## field entry + scripted hold (logic)
- **scope:** stage 0x8010637C GAME field entry via hut-entry replay; guest RAM+scratchpad
- **status:** verified
- **frames:** 8220
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_SBS_MODE=gameplay PSXPORT_SBS_AUTONAV=1 PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-alt.pad ./scratch/bin/tomba2_port
- **evidence:** ffec2399 2026-07-15 — 8220 frames 0-diff; both cores field-rendering @f216
- **owner:** game/render/render_walk.cpp (sceneNative reads this state)
- **notes:** covers field ENTRY + scripted-caught hold (autonav did NOT reach free-roam control); free-roam + sceneNative RENDER correctness (eyeball) still uncovered — see portmap field-world

## field-world
- **status:** verified
- **frames:** 1800
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_EXIT_FRAME=1800 PSXPORT_VK_HEADLESS=1 PSXPORT_SKIP_FMV=1 PSXPORT_PAD_REPLAY=replays/boot-smoke/general-session.pad
- **evidence:** SBS full 0-diff x3: general-session 600f (8.9s) + 1800f (22.9s), hut-entry 1200f (14.5s), all 'A/B identical', 0 DIVERGENCE

## framework-agnostic-p1.7c
- **status:** verified
- **frames:** 450
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_VK_HEADLESS=1 PSXPORT_AUTO_SKIP=1 ./scratch/bin/tomba2_port
- **evidence:** 58fc5f76

## gte-transform3-84250
- **scope:** GTE 3-vertex rotate+pack FUN_80084250
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=14675 oracle=14675 (balanced). Plus a headless frame at f1400 byte-identical to the pre-change capture, which matters because this body drives the GTE and SBS does not compare COP2 state.
- **owner:** game/math/wide_re_gte_transform3.cpp GteTransform3::rotate3AndPackIr

## id-routed-offset-point-122bf4
- **scope:** 0x80122BF4 pin a point above a linked node in its rotated frame
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=2819 oracle=2819 balanced; port_check PASS.

## input-set-voice-volume-92e3c
- **scope:** 0x80092E3C SPU voice-attribute volume stage
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=2327 oracle=2327 balanced.

## libapi-setintrmask-85c9c
- **scope:** libapi SetIntrMask FUN_80085C9C (I_MASK swap via libapi's hw-pointer table)
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=19594 oracle=19594 (balanced). Plus a non-SBS render check to f1400 — the scene draws correctly, which matters because this writes I_MASK and the framework has just gained real interrupt delivery.
- **owner:** game/core/libapi_intr.cpp LibapiIntr::setIntrMask

## LibapiIntr::runVblankCallbacks (FUN_80086288)
- **scope:** game/core/libapi_intr.cpp
- **status:** verified
- **frames:** 400
- **gate:** PSXPORT_MIRROR_VERIFY=0x80086288 PSXPORT_DEBUG=mirror-verify PSXPORT_NOAUDIO=1 PSXPORT_VK_HEADLESS=1 PSXPORT_REPL=1 ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE (repl: run 400)
- **evidence:** scratch/logs/mv_86288.log — 800 invocations, 0 mismatches (RAM touched-byte union + full scratchpad + v0/v1/s0-s7/gp/sp/fp/ra + hi/lo). Instrument validated in BOTH directions: an r16/r17 spill-swap built in an isolated copy MISMATCHes at invocation #1 on the sp+16/sp+20 stack bytes (scratch/logs/mv_negctl.log).
- **owner:** libapi/VBlank
- **notes:** Per-invocation mirror gate, NOT an SBS run: verified over the boot+title window this run reaches. The 8 callback slots at 0x800ABDC0 are all NULL in that window, so the indirect dispatch arm is UNEXERCISED — the counter bump, the 8-slot walk, the frame spills and the register/stack state are what 800 passes prove.

## libc-memcpy-9a3e0
- **scope:** libc memcpy FUN_8009A3E0 (null-dst guard, signed length)
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=1542 oracle=1542 (balanced).
- **owner:** game/core/str.cpp Str::copyBytes

## libgpu-dma-status-reset-82c68
- **scope:** libgpu GPU-DMA status-block reset FUN_80082C68
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=1500 oracle=1500 (balanced — once per frame, as expected for a per-frame DMA status reset).
- **owner:** game/render/wide_re_libgpu_leaves.cpp libgpuDmaStatusReset

## libgpu-setdrawenv-81fb0
- **scope:** render
- **status:** verified
- **frames:** 1800
- **gate:** printf 'run 300\nquit\n' | PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_SBS_MODE=full PSXPORT_DEBUG=ovhit ./scratch/bin/tomba2_port
- **evidence:** scratch/logs/sdenv_sbs.log + sdenv_sbs_psxr.log: [sbs] A/B identical at every 30-frame checkpoint f0..f1800, both with pc_render and with PSXPORT_RENDER_PSX=1. Override provably ACTIVE: single-core psx_render run (scratch/logs/sdenv_native.log, exit 0) shows [ovhit] 0x80081FB0 LibgpuDrawEnv::setDrawEnv native=300 oracle=0 over 300 frames, i.e. it fires once per frame. CAVEAT: the SBS runs end at f1800 in an UNRELATED pre-existing pc_render crash (Render::fieldObjectsRender, game/render/render_walk.cpp, unmapped read8 @0x07035D41 from a stale node link off head 0x800FB168/0x800F2624/0x800F2738 during renderAttract), so the SBS ovhit table never printed — the matched-count evidence is from the single-core run, the 0-diff evidence from the SBS runs.
- **owner:** LibgpuDrawEnv::setDrawEnv (game/render/libgpu_draw_env.cpp)
- **notes:** port_check PASS. The crash cannot be attributable to this port: core B runs the pure gen_func_80081FB0 and A/B guest RAM was byte-identical through the crash frame, so the pointer fieldObjectsRender chases is the substrate's own.

## libgpu-setdrawmode-83de0
- **scope:** libgpu SetDrawMode FUN_80083DE0 (DR_TPAGE/DR_TWIN header builder)
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=11323 oracle=11323 (balanced, so the compare covers it on both legs). The gate is load-bearing for the argument correction: the draft masked a1 where the guest masks a3, which would have written a different DR_TPAGE mode word into guest packets and diverged.
- **owner:** game/render/wide_re_libgpu_leaves.cpp libgpuSetDrawMode

## math-rotmatsoftinverse-84a80
- **scope:** software Euler rotation-matrix sibling FUN_80084A80
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit native=14185 oracle=14185. Load-bearing: all nine matrix elements are written to GUEST memory, so any transcription error in the derived formulas diverges immediately. Frame at f1400 also byte-identical to baseline.
- **owner:** game/math/gte_math.cpp Math::rotMatSoftInverse

## mtx-identity-51794
- **scope:** MR_init identity-matrix leaf FUN_80051794, registry-wired
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence, with the address executing 20269 times on the native leg. ovhit reports native=20269 oracle=26738; the 6469 gap is calls from already-native callers reaching Mtx::identity directly rather than through the guest fn, NOT a divergence (see the banner in mtx.cpp).
- **owner:** game/math/mtx.cpp Mtx::identity + Mtx::registerOverrides

## node-lifecycle-sm-40558
- **scope:** 0x80040558 placed scene-prop behaviour handler (RENAMED: was 'per-node lifecycle state machine'; owner is now PlacedPropSm::step)
- **status:** verified
- **frames:** 6000
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_VK_HEADLESS=1 PSXPORT_SBS_MODE=full PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=6000 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** RE-RUN 2026-07-30 against the REBUILT body (the previous verified run gated the retired port_gen transcript, so that evidence did not carry over): 200/200 A/B-identical checkpoints over 6000 frames, zero RAM+scratchpad divergence, exit 0; ovhit native=82544 oracle=82544 balanced — the same execution count the transcript recorded. Also port_check PASS (frame 24/24, all 32 call sites' ra constants + targets, full 24-store width sequence). scratch/logs/pp_sbs.log
- **owner:** game/ai/placed_prop_sm.cpp PlacedPropSm::step + PlacedPropSm::registerOverrides

## objlist-walk2-case0-3bdac
- **scope:** 0x8003BDAC objListWalk2 jump-table case 0/15 trampoline
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** Batch gate: 50/50 A/B-identical checkpoints, zero divergence; ovhit native=9060 oracle=9060 (balanced).

## overlay-gt3gt4-submitblock
- **scope:** A00 field submit-block dispatcher FUN_80146478
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** 50/50 A/B-identical checkpoints, zero divergence; ovhit 0x80146478 native=76378 oracle=76378 (and both leaves likewise) so the green gate provably COVERS this address
- **owner:** game/render/overlay_gt3gt4.cpp OverlayGt3Gt4::submitBlock

## pad-sample-button-mask-524b4
- **scope:** 0x800524B4 port-0 controller button-mask sampler
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=1500 oracle=1500 balanced.

## Panel taps FUN_8004FFB4/8005019C (gen + native quad push)
- **status:** verified
- **frames:** 20970
- **gate:** SBS-full watch-cut 0-diff f20970 + combat f5970 (taps run gen bodies; push half host-only)
- **evidence:** 77b7bcdb

## render-billboard-c5f8
- **status:** verified
- **frames:** 360
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-alt.pad
- **evidence:** ovhit 0x8003C5F8 native=8 oracle=8; A/B identical f0..f360

## render-mat-847f0
- **status:** verified
- **frames:** 360
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-alt.pad
- **evidence:** ovhit rotMatSoft oracle=8; MAT_ROTZ byte-identical f0..f360

## ScreenFade leaf tap FUN_8007E9C8
- **status:** verified
- **frames:** 23850
- **gate:** same two legs; THUNK_FORCE_GEN A/B exonerated tap; ovhit native=32 newgame->narration
- **evidence:** 7a282422

## script-interp-advance
- **status:** verified
- **frames:** 41280
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 ./scratch/bin/tomba2_port
- **evidence:** 7cb7bee

## spawn-inner-dispatch-child-13892c
- **scope:** 0x8013892C spawn the assembly's companion node
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=6 oracle=6 balanced; port_check PASS.

## str-compare-bytes-9a640
- **scope:** 0x8009A640 byte compare
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=520 oracle=520 balanced.

## substate-arm-child-pair-131134
- **scope:** 0x80131134 pending child-pair arm
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** Batch gate: 50/50 A/B-identical checkpoints, zero divergence; ovhit native=16728 oracle=16728 (balanced).

## substate-tick-child-osc-1316cc
- **scope:** 0x801316CC child-oscillator driver loop
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** Batch gate: 50/50 A/B-identical checkpoints, zero divergence; ovhit native=16728 oracle=16728 (balanced).

## substate-visibility-gate-130ac4
- **scope:** 0x80130AC4 multi-point visibility gate
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG=ovhit PSXPORT_SBS_EXIT_FRAME=1500 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad ./scratch/bin/tomba2_port
- **evidence:** Batch gate: 50/50 A/B-identical checkpoints, zero divergence; ovhit native=16728 oracle=16728 (balanced).

## substate0tick-12f494
- **scope:** 0x8012F494 orchestrator node[5]==0 sub-state tick
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=16680 oracle=16680 balanced; port_check PASS.

## swing-stroke-group-tick-130d5c
- **scope:** 0x80130D5C per-sub-part oscillator
- **status:** verified
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=11152 oracle=11152 balanced; port_check PASS.

## TileGridLayer scrollStep+emit (0x8011534C/0x80115598)
- **status:** verified
- **frames:** 20820
- **gate:** SBS-full combat 0-diff f5850 + watch-cut 0-diff f20820; ovhit native=oracle=3878 both addrs
- **evidence:** cd278ce4

## ui-ft4-plain-quad-7e2f8
- **scope:** 0x8007E2F8 POLY_FT4 UI vertex-layout case 0
- **status:** verified
- **frames:** 1500
- **gate:** PSXPORT_SBS_MODE=full ... PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** Batch gate: 50/50 A/B-identical, zero divergence; ovhit native=349 oracle=349 balanced.

## ActorTomba::actionHandler8005EF48 (FUN_8005EF48)
- **status:** partial
- **gate:** port_check equivalence (verbatim) + wired 0-diff; mode unreached by autonav — runtime-unexercised
- **evidence:** 58809b1c

## ActorTomba::actionHandler8005F1B0 (FUN_8005F1B0)
- **status:** partial
- **gate:** port_check equivalence (verbatim) + wired 0-diff, but mode unreached by combat/field/cutscene autonav — runtime-unexercised
- **evidence:** 71b5d764

## ActorTomba::actionHandler800660AC (FUN_800660AC)
- **status:** partial
- **gate:** port_check equivalence (verbatim) + wired 0-diff; mode unreached by autonav — runtime-unexercised
- **evidence:** 58809b1c

## Gauge text-row tap FUN_8004EB94
- **status:** partial
- **gate:** registered, 2-leg 0-diff, but ovhit 0/0 (no gauge item in autonav) — host push math unexercised; needs gauge-popping drive + USER eyeball
- **evidence:** 7a48eb15

## Icon glyph tap FUN_80078988
- **status:** partial
- **gate:** 2-leg 0-diff with tap registered; icon strings not exercised in autonav legs — needs an icon-showing drive + USER eyeball
- **evidence:** 916ddfc0

## libapi-clear-words-86320
- **scope:** 0x80086320 word-fill helper
- **status:** partial
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=1 oracle=1 balanced — ONE execution (boot-time init), so correct where gated but barely exercised.

## libapi-init-vblank-cbs-86230
- **scope:** 0x80086230 VSync callback table init
- **status:** partial
- **frames:** 1500
- **gate:** SBS full, seesaw-weight.pad
- **evidence:** 50/50 A/B-identical; ovhit native=1 oracle=1 balanced — ONE execution (boot-time init), so correct where gated but barely exercised.

## options-page-backdrop
- **scope:** FUN_8007FC24 — the OPTIONS page's full-screen POLY_G4 backdrop packet (OptionsPage::pushBackdrop)
- **status:** partial
- **gate:** python3 external/psxport/tools/port_check.py game/ui/options_page.cpp
- **evidence:** port_check PASS vs gen_func_8007FC24 (frame sizes, call sites, store-width sequence); AND the psx_render leg, which rasterizes the packet this port writes, is 0/76800 against the pre-port build at replays/bugs/ingame-options-page.pad f1160
- **owner:** game/ui/options_page.cpp
- **notes:** NOT yet run under SBS full — the two proofs above are static equivalence plus a pixel-exact readback of the packet through the PSX rasterizer, not a guest-RAM byte compare. Promote to verified with an SBS-full run that reaches the page.

## sequencer-voice-state-flush-931c0
- **scope:** 0x800931C0 SPU voice-state flush
- **status:** partial
- **frames:** 6000
- **gate:** PSXPORT_SBS_MODE=full PSXPORT_SBS_EXIT_FRAME=6000 PSXPORT_PAD_REPLAY=replays/bugs/seesaw-weight.pad
- **evidence:** SBS 200/200 A/B-identical over 6000 frames, zero divergence, ovhit native=1 oracle=1 balanced. The low count is now EXPLAINED and is not a window artefact: this function runs 12,001 times per 6000 frames on the DEFAULT pc_skip=true path, but SBS forces pc_skip=false on both cores, and on the faithful path it runs once. So the byte-exact gate covers one execution of a function that is hot in ordinary play. Still PARTIAL: correct where gated, but the hot path is ungated by this instrument.

## actor-zoned-zoneclassify-145c78
- **scope:** 0x80145C78 zone-band classifier
- **status:** untested
- **evidence:** Wired and build-clean, but ovhit reports native=0 oracle=0 on the seesaw-weight replay — NEVER REACHED. The batch's 50/50 A/B-identical result says nothing whatsoever about this address. Needs a scene that drives ActorZonedAttacker::gateCheck (FUN_8014047C), its only caller.

## arm-child-pair-by-angle-131768
- **scope:** 0x80131768 arm a sub-part pair by angle
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit native=0 oracle=0 — never reached by this replay, so the 50/50 result says nothing about it.

## chain-cmdclear-1313c4
- **scope:** 0x801313C4 pending-command clear
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit native=0 oracle=0 on the seesaw-weight replay — never reached, so the 50/50 batch result says nothing about this address.

## chain-substate2-12fd88
- **scope:** 0x8012FD88 sub-state 2 tick
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit native=0 oracle=0 on the seesaw-weight replay — never reached, so the 50/50 batch result says nothing about this address.

## chain-substate3-130524
- **scope:** 0x80130524 sub-state 3 tick
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit native=0 oracle=0 on the seesaw-weight replay — never reached, so the 50/50 batch result says nothing about this address.

## cull-set-scale-swing-133610
- **scope:** 0x80133610 swing profile/scale setter
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit native=0 oracle=0 — never reached by this replay.

## render-billboard-c788
- **status:** untested
- **evidence:** NOT verifiable by the current replay library: PSXPORT_DEBUG=ovhit shows 0x8003C788 native=0 oracle=0 NEVER HIT across general-session and hut-entry, incl. a 4000-frame SBS run. The SBS 0-diff result for those runs is vacuous for this unit — it never executes. Needs a scenario that actually dispatches C788 before any gate means anything.

## rtm-xsweepcycle-124328
- **scope:** 0x80124328 release-trigger X-sweep cycle
- **status:** untested
- **evidence:** port_check PASS and build-clean, but ovhit reports native=0 oracle=0 — the seesaw-weight replay never reaches it, so the batch's 50/50 result says nothing about this address.

## ui-ft4-flip-xy-8007E4A8
- **scope:** 0x8007E4A8 POLY_FT4 UI layout case
- **status:** untested
- **evidence:** port_check UNPROVABLE (tail-jump, no ra on either side — same as the already-owned case 0), build-clean, but ovhit native=0 oracle=0: this replay only ever takes layout case 0 (349 hits). Needs a scene using another layout mode.

## ui-ft4-v-mirrored-8007E410
- **scope:** 0x8007E410 POLY_FT4 UI layout case
- **status:** untested
- **evidence:** port_check UNPROVABLE (tail-jump, no ra on either side — same as the already-owned case 0), build-clean, but ovhit native=0 oracle=0: this replay only ever takes layout case 0 (349 hits). Needs a scene using another layout mode.

## ui-ft4-v-mirrored-plus-8007E584
- **scope:** 0x8007E584 POLY_FT4 UI layout case
- **status:** untested
- **evidence:** port_check UNPROVABLE (tail-jump, no ra on either side — same as the already-owned case 0), build-clean, but ovhit native=0 oracle=0: this replay only ever takes layout case 0 (349 hits). Needs a scene using another layout mode.

## ui-ft4-x-mirrored-8007E36C
- **scope:** 0x8007E36C POLY_FT4 UI layout case
- **status:** untested
- **evidence:** port_check UNPROVABLE (tail-jump, no ra on either side — same as the already-owned case 0), build-clean, but ovhit native=0 oracle=0: this replay only ever takes layout case 0 (349 hits). Needs a scene using another layout mode.

## Billboard picture dual-emit (rq_push_ft4_record @ billboardEmit/submitQuad)
- **status:** n/a
- **gate:** host-only queue push (zero guest writes); 2-leg 0-diff f5520/f20610 with pushes live; USER play-test pending (#65)
- **evidence:** 8988b389

## Font::glyphQueuePush (glyphEmit dual-emit host half)
- **status:** n/a
- **gate:** host-only queue push, zero guest writes; glyphEmit faithful body previously verified
- **evidence:** 0c711055

## pause-menu-chrome
- **status:** n/a
- **evidence:** pc_render overlay: host memory only, taps run gen and write no guest byte

## Render::a0fVortexRender (area 15 portal producer)
- **scope:** render
- **status:** n/a
- **owner:** game/render/fx_vortex.cpp
- **notes:** pc_render overlay: host quads only, never writes guest RAM (the guest still runs ov_a0f_gen_801143C4). SBS-full A/B identical through f16770 with it installed.

## Render::dialogTextNative
- **scope:** field/hut dialog TEXT producer (pc_render overlay)
- **status:** n/a
- **gate:** RETIRED 916ddfc0 — superseded by the FUN_8007CC00 tap
- **evidence:** 916ddfc0
- **owner:** game/render/render_walk.cpp
- **notes:** read-only pc_render producer — writes ONLY host memory, never guest RAM; parity N/A by construction (DisplayPassGuard enforces). Correctness = USER eyeball, not SBS.

## Render::fieldHudMinimap (areas 2/7 minimap producer)
- **scope:** render
- **status:** n/a
- **owner:** game/render/minimap.cpp
- **notes:** pc_render overlay: two host 2D quads at RQ_OVERLAY, never writes guest RAM. SBS-full A/B identical through f16770.

## world-line-rope
- **status:** n/a
- **evidence:** pc_render display-pass producer: reads guest state, writes only host memory (no guest write); SBS-full on replays/bugs/bucket-softlock.pad first divergence unchanged at 0x801FE808 f175 (pre-existing, kanban #61)
