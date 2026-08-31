# Project state

## Comparison baseline

The baseline is the unmodified PlayStation releases of *Tomba!* and *Tomba! 2* running on original
hardware or through a PS1 emulator, with console execution, 4:3 framing, and original presentation
cadence. Tomba Engine intends native game-engine ownership for both titles; Tomba! 2 additionally
targets true widescreen and interpolated presentation, while Tomba! 1 intentionally adds widescreen
only.

This is the factual capability inventory for both titles in this repository. It does not infer one
title's coverage from the other. Epic intent is in `project-goals.md`, atomic work in `issues/`, and
ownership/placement in `codemap.md`.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Default Tomba! 2 product reaches a usable PC-native game | partial | — | G001 |
| S002 | Tomba! 2 behavior is independently compared against the original | partial | S001 | G001 |
| S003 | Tomba! 2 game behavior is owned by readable native subsystems | partial | S001, S002 | G001 |
| S004 | Tomba! 2 picture is produced completely from game-owned scene state | partial | S001, S003 | G002 |
| S005 | Tomba! 2 true widescreen covers world visibility and 2D layout | partial | S004 | G002 |
| S006 | Tomba! 2 interpolation covers moving camera, objects, and effects | partial | S004 | G002 |
| S007 | Tomba! 2 accepts native player input through representative gameplay | partial | S001, S003 | G001 |
| S008 | Tomba! 1 selected executable and disc provenance are established | partial | — | G003 |
| S009 | Tomba! 1 evidence scaffold is integrated in the combined Clang build | verified | S008 | G003 |
| S010 | Tomba! 1 actual product boots, renders, accepts input, and reaches gameplay | missing | S008, S009 | G003 |
| S011 | Tomba! 1 true widescreen works in the actual product | missing | S010 | G004 |
| S012 | Tomba! 1 and Tomba! 2 game-engine implementations are isolated | verified | — | G003 |
| S013 | Tomba! 1 exposes widescreen only and no unrelated enhancement modes | verified | S012 | G004 |
| S014 | Tomba! 2 sound effects and music work throughout the game | partial | S001, S003 | G001 |
| S015 | Tomba! 2 saves, reloads, and survives a full restart | partial | S001, S003 | G001 |
| S016 | Tomba! 2 movies play correctly on the native path | partial | S001, S003 | G001 |
| S017 | Tomba! 2 area and scene transitions work throughout the game | partial | S001, S003 | G001 |

## Current focus

S005 is the current focus: the exact combined product now reaches live 16:9 gameplay and the
title-specific compositor fills the added side canvas without changing the authored central picture.
Matched 4:3/wide controls and representative scene, culling, and HUD-anchor coverage remain.

## Capability details

### S001 — Default Tomba! 2 product: partial

The repository has a shipping `tomba2_port` target and a zero-argument launcher for the intended
PC-native execution/render route. On exact psxport `99a42aa3`, the actual headless product loaded the
retail executable and disc, traversed the title, entered live gameplay, committed 620/620 frame
ledgers with zero dropped layers, captured frames 400 and 600, and exited at its requested native
frame bound. This is direct combined-product evidence, not a focused selftest.

The current source moves that previously framework-shaped transaction into
`TombaFrameDriver::stepFrame`: the runtime creates it before boot, `bootInit` is finite, and the driver
owns the measured input, timing, scheduler, render-submit, and single presentation-fence order. The
measured Tomba! 2 libetc entry `0x80085900` remains a fatal guest-VSync trap; the product gate rejects
both `VSync: timeout` and the trap diagnostic. A fresh Clang build of this exact source completed a
bounded 620-frame native run, entered GAME at frame 25 and free-roam at frame 216, reconciled 620/620
presentation fences with zero dropped layers, captured visibly coherent full-width frames 400 and 600,
and exited normally without a guest-VSync violation, timeout, or recomp miss. The captured PPM
SHA-256 values are `7c4a8cf1f04a0e7744054e37c896f013bef2c92a2e389d24cf7fd57808d1b4d6`
and `bdf99b753ffb57ec6c2f0764a05a8e4b141263b696039e1982f7b08f71aa21df`. Evidence:
`scratch/logs/tomba2-extracted-autoskip.log` and `scratch/screenshots/present_{400,600}.ppm`.

Gap: this run used the documented direct product command rather than the zero-argument launcher, and
it did not exercise representative input, audio, saving, transitions, or restart.

### S002 — Independent Tomba! 2 comparison: partial

The byte-exact SBS mechanism compares native and recompiled cores and its canary demonstrates that it
can report a mismatch.

The separate `PSXPORT_DUALVIEW` presentation diagnostic is currently refused by `TombaRuntime`.
Issue #4 records why: the shared SDL_GPU backend's `gpu_vk_select_target` is a no-op and
`gpu_vk_target_count` always returns zero, so the previously advertised right-hand PSX pane did not
exist. A 300-frame live run proved the title's rewind/re-render transaction itself did not crash, but
its screenshot remained only the native target; that is not side-by-side evidence. The current
product exits 1 before its frame loop with that exact backend reason; the bounded refusal is recorded
in `scratch/logs/tomba2-extracted-dualview-refusal.log`.

Gap: both cores derive from this project. The planned independent Beetle reference, BIOS/HLE state
reconciliation, representative full-game comparisons, and shared-backend dual-target implementation
are incomplete.

### S003 — Native Tomba! 2 behavior ownership: partial

The root `game/` tree owns substantial engine, world, player, scene, UI, audio, and render behavior;
`docs/code-map.md` indexes address-level owners and `docs/port-map.md` records the RE frontier.
The title-local `TombaFrameDriver` now owns the whole per-frame game transaction through the runtime's
polymorphic factory instead of depending on a Tomba-shaped framework loop. Its composed `AutoDrive`
and `FrameDiagnostics` objects also own the stage/cutscene-aware input policy and every Tomba guest-
layout frame probe; psxport retains only generic iteration, budgets, replay/debug transport, dumps,
and result accounting. A bounded real REPL run reached GAME on frame 25, requested the next prompt
from the title driver after that completed transition frame, resumed the generic prompt at frame 26,
then completed 326/326 presentation fences with zero dropped layers. Evidence:
`scratch/logs/tomba2-extracted-newgame.log`.

Gap: substrate functions and known unowned behavior remain; complete native engine ownership and an
independent behavior proof have not been reached.

### S004 — Tomba! 2 game-state picture: partial

The native renderer and many scene/UI/effect producers draw from owned game state. The current
`FUN_8013ED08` and `FUN_80078988` milestones add a rigid effect mesh and icon-glyph picture owner.
`TombaRuntime::renderCapabilities()` explicitly returns
`RenderCapabilities::interpolatedNative()`, retaining the title's native player path and temporal
presentation while the remaining legacy callbacks migrate independently.

The exact 620-frame run visually reached both title and gameplay through the Native route. Its run-end
ledger attributed 552,424 primitives to native producers across 13 re-earned rows, with 4,589
undeclared native primitives remaining.

Gap: `docs/unported-render-inventory.md` still records missing effect-mesh controllers and other
unverified layers, and the undeclared primitives prove producer attribution is incomplete.

### S005 — Tomba! 2 widescreen: partial

Owned projection, camera, culling-margin, backdrop, terrain, and 2D queue paths contain wide behavior.
The exact controlled `aspect=1` run produced a 960-pixel-wide gameplay picture at frame 600 with
additional world content across the full output width. The title-specific wide compositor now fills
frame 400's added side canvas with dim mirrored edge continuations while preserving the original
718x520 central picture pixel-for-pixel. A seam-corrected real-product run completed 620/620 native
frame fences, emitted 900 `pc/title-wide-margins` primitives across 450 title frames, and left the
frame-600 gameplay control pixel-identical.

Gap: issue #3's implementation is verified but not landed. Matched 4:3/wide controls and representative
scene, edge-visibility, culling, and HUD-anchor coverage remain incomplete.

### S006 — Tomba! 2 interpolation: partial

The Fps60/EffectLerp presentation owners capture and interpolate camera, object, backdrop, and effect
state through native presentation passes. The exact product loaded `fps60=1`, completed its 620-frame
loop, and presented coherent inspected title/gameplay frames without dropping a ledger layer.

Gap: two still images cannot prove temporal smoothness. The unported-render inventory and
producer-specific notes still name stepped, snapped, cold, or unverified layers; complete
representative temporal coverage is not established.

### S007 — Tomba! 2 input: partial

The native input subsystem exists and focused or replay evidence reaches controlled game flow.

Gap: an observed end-to-end run of the current default product through representative player-driven
gameplay is absent.

### S008 — Tomba! 1 executable/disc provenance: partial

`titles/tomba1/executable.json` and its verifier establish 15 whole-file and PS-X header facts for a
measured `SCUS_942.36`, including an altered-byte negative control.

Gap: issue 1 records the missing selected-disc `SYSTEM.CNF` proof and reproducible extraction path.

### S009 — Tomba! 1 combined evidence scaffold: verified

Evidence: the repository root includes `titles/tomba1/cmake/tomba1_port.cmake`; its scaffold depends
on the real shared `psxport` library, and the combined Clang CTest graph runs the title identity
selftest plus positive/negative isolation checks without publishing a product executable.

### S010 — Tomba! 1 product: missing

Missing capability: no generated substrate, independent execution boundary, `tomba1_port` target,
launcher, visible product frame, or input-driven gameplay exists.

### S011 — Tomba! 1 widescreen: missing

Missing capability: no executable-grounded projection, visibility-culling, edge-coverage, or 2D
layout owner has been recovered or exercised in a running Tomba! 1 product.

### S012 — Cross-title engine isolation: verified

Evidence: the Tomba! 1 fragment compiles no root `game/` or `generated/` source, its required owners
live under `titles/tomba1/`, and `verify_title_isolation.py` enforces the cross-title include/token and
1,200-line structure boundaries with positive and negative controls.

### S013 — Tomba! 1 widescreen-only scope: verified

Evidence: `titles/tomba1/enhancement_scope.json` contains only `{"widescreen": true}`; the same
shipping isolation checker rejects any additional mode and source/CMake registrations for
interpolation, temporal history, native rendering, or 60fps.

### S014 — Tomba! 2 audio: partial

The native audio and CD/XA subsystems exist and have focused evidence.

Gap: sound effects and music have not been observed across representative full-game progression.

### S015 — Tomba! 2 saves and restart: partial

Save and menu owners exist in the native engine.

Gap: no current shipping-product run proves save, process restart, and successful reload end to end.

### S016 — Tomba! 2 movies: partial

FMV and CD owners exist and have focused coverage.

Gap: representative movies have not been observed through the current default native product.

### S017 — Tomba! 2 transitions: partial

Scene and transition subsystems exist and reached flow includes title-to-gameplay transitions.

Gap: representative area and scene transitions across the full game remain unverified.
