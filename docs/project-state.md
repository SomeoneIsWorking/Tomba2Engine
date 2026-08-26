# Project state

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
| S007 | Tomba! 2 full-game input, audio, saves, movies, and transitions work | partial | S001, S003 | G001 |
| S008 | Tomba! 1 selected executable and disc provenance are established | partial | — | G003 |
| S009 | Tomba! 1 evidence scaffold is integrated in the combined Clang build | verified | S008 | G003 |
| S010 | Tomba! 1 actual product boots, renders, accepts input, and reaches gameplay | missing | S008, S009 | G003 |
| S011 | Tomba! 1 true widescreen works in the actual product | missing | S010 | G004 |
| S012 | Tomba! 1 and Tomba! 2 game-engine implementations are isolated | verified | — | G003 |
| S013 | Tomba! 1 exposes widescreen only and no unrelated enhancement modes | verified | S012 | G004 |

## Current focus

S005 is the current focus: the exact combined product now reaches live 16:9 gameplay, while the title
capture remains pillarboxed; issue #3 owns that first directly observed wide-composition gap.

## Capability details

### S001 — Default Tomba! 2 product: partial

The repository has a shipping `tomba2_port` target and a zero-argument launcher for the intended
PC-native execution/render route. On exact psxport `99a42aa3`, the actual headless product loaded the
retail executable and disc, traversed the title, entered live gameplay, committed 620/620 frame
ledgers with zero dropped layers, captured frames 400 and 600, and exited at its requested native
frame bound. This is direct combined-product evidence, not a focused selftest.

Gap: this run used the documented direct product command rather than the zero-argument launcher, and
it did not exercise representative input, audio, saving, transitions, or restart.

### S002 — Independent Tomba! 2 comparison: partial

The byte-exact SBS mechanism compares native and recompiled cores and its canary demonstrates that it
can report a mismatch.

Gap: both cores derive from this project. The planned independent Beetle reference, BIOS/HLE state
reconciliation, and representative full-game comparisons are incomplete.

### S003 — Native Tomba! 2 behavior ownership: partial

The root `game/` tree owns substantial engine, world, player, scene, UI, audio, and render behavior;
`docs/code-map.md` indexes address-level owners and `docs/port-map.md` records the RE frontier.

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
additional world content across the full output width. Frame 400's title/menu picture remained
centered with black side margins and only a 718-pixel non-black span.

Gap: issue #3 owns the observed title composition. Matched 4:3/wide controls and representative scene,
edge-visibility, culling, and HUD-anchor coverage remain incomplete.

### S006 — Tomba! 2 interpolation: partial

The Fps60/EffectLerp presentation owners capture and interpolate camera, object, backdrop, and effect
state through native presentation passes. The exact product loaded `fps60=1`, completed its 620-frame
loop, and presented coherent inspected title/gameplay frames without dropping a ledger layer.

Gap: two still images cannot prove temporal smoothness. The unported-render inventory and
producer-specific notes still name stepped, snapped, cold, or unverified layers; complete
representative temporal coverage is not established.

### S007 — Tomba! 2 full-game systems: partial

Input, audio, FMV/CD, scene, save/menu, and transition subsystems exist and several have focused and
replay evidence.

Gap: an observed end-to-end run of the current default product through representative gameplay,
saving, movies, area transitions, and restart is absent.

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
