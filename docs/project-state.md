# Project state

Baseline: the unmodified PlayStation releases of *Tomba!* and *Tomba! 2*, and this repository's
pre-migration native/offline-translated hybrid. Every user-visible delta from that baseline is one of
the capability rows below (widescreen = S005/S011, 60 fps interpolation = S006, loading removal =
S023/S024, game-state picture = S004). Epic intent is in `project-goals.md`, migration order in
`migration.md`, atomic work in `issues/`, placement in `codemap.md`, and which reverse-engineering
steps are ground-truth-ready and which are not in `re-frontier.md`.

Current focus: S001 — issue 0005 must prove one resident and one colliding-overlay override, each
scoped back into the original guest body, before the recorded gameplay frontier is restored.

| ID | Capability | State | Evidence or gap |
|---|---|---|---|
| S001 | Tomba! 2 reaches representative gameplay as a native/Lightrec product | blocked | Headless launch runs 2500 frames of the seaside field with zero faults and zero Lightrec fallback, past the f870 water-jet abort (issues 0026 and 0022/kanban-120 closed); representative gameplay unverified (issue 0005). |
| S002 | Tomba! 2 behaviour compared independently against the original | partial | `tools/oracle_compare.py` compares 34 title-owned RAM checkpoints against the Beetle reference, and `tools/picture_oracle.py` the presented picture; known picture deltas are tracked in `issues/0019`. |
| S003 | Tomba! 2 game behaviour owned by readable native subsystems | partial | Native owners exist across `game/`; 42 declared-at-overlay-address overrides that never installed are fixed and refused by abort (issue 0015, closed). |
| S004 | Tomba! 2 picture produced completely from game-owned scene state | partial | `renderpath psx` still needs the substrate path for some producers; guest-GTE producers remain in the item menu (issue 111 card). The water jet's mesh branch is native and interpolating again (kanban-120, issue 0022). |
| S005 | Tomba! 2 true widescreen covers world visibility and 2D layout | partial | `aspect=1 wide_engine=1 native_width=320 render_width=428`; sky/sea seams at 16 px multiples and the 3-px picture shift are still open (`issues/0012`, `issues/0019`). |
| S006 | Tomba! 2 interpolation covers moving camera, objects and effects | partial | Per-object interpolated 60 fps on; real-vs-interp splits still leave stale/ahead entities (issue 0009 open). |
| S007 | Tomba! 2 accepts native player input through representative gameplay | partial | Held pad input walks and jumps Tomba in the seaside field for 402 frames with the reference agreeing; other areas, menus and combat undriven. |
| S008 | Tomba! 1 selected executable and disc provenance established | verified | `SCUS_942.36`, 559,104 B, SHA-1 `81cbc79f0230aeb4252e058039f47ac95a777f5a`, entry `0x8006B58C`, text `[0x80010000,0x80098000)`, `SYSTEM.CNF` agreement before publication. |
| S009 | Tomba! 1 identity, isolation and independent startup evidence exist | verified | Title-local identity + isolation checks and a 35-field CRT0 boundary (`titles/tomba1/tools/verify_executable.py`, `compare_crt0_boundary.py`). |
| S010 | Tomba! 1 reaches representative gameplay as a native/Lightrec product | blocked | Pre-migration run reached SCEA/Whoopee Camp presentation; next boundary is the guest stream task budget (issue 0008 open). |
| S011 | Tomba! 1 true widescreen works in the actual product | missing | `titles/tomba1/game/render/widescreen_projection.cpp` exists; no product run yet (depends on S010). |
| S012 | Tomba! 1 and Tomba! 2 game-engine implementations are isolated | verified | Tomba! 1 owners live only under `titles/tomba1/` and import no root `game/` source. |
| S013 | Tomba! 1 exposes widescreen only, no unrelated enhancement modes | verified | `titles/tomba1/enhancement_scope.json` is `{"widescreen": true}`; the title surface rejects 60 fps, native rendering and lerp. |
| S014 | Tomba! 2 sound effects and music work throughout the game | partial | Native audio and CD/XA owners exist; the per-vblank sequencer tick runs again on the slots its own trampoline reads (issue 0026), but not yet observed through representative Lightrec gameplay. |
| S015 | Tomba! 2 saves, reloads and survives a full restart | partial | Save and menu owners exist; no product run proves save → restart → reload. |
| S016 | Tomba! 2 movies play correctly | partial | FMV/CD owners exist with pre-migration coverage; unobserved on the Lightrec product. |
| S017 | Tomba! 2 area and scene transitions work throughout the game | partial | Title-to-gameplay transition recorded; representative transitions unverified (card `kanban-108`). |
| S018 | Both titles removed their offline guest-source product paths | verified | No emitted guest source, generator, static dispatch or generation-only selftest remains; both CMake graphs build native/Lightrec products. |
| S019 | Linux x86-64 asset-free native/Lightrec CI builds and tests the repository | verified | Hosted `Linux x86-64 native/Lightrec contract` job runs `tools/verify_ci.py` (build + product tests + clang-format/tidy/cpp policy). |
| S020 | Windows x86-64 native/Lightrec CI builds and tests the repository | missing | No Windows product composition or hosted build; CMake graph and psxport presentation unproven there. |
| S021 | macOS x86-64 and arm64 native/Lightrec CI builds and tests the repository | missing | No hosted macOS build; executable-memory publication, icache coherence, ABI and SDL presentation unqualified on arm64. |
| S022 | Android arm64-v8a native/Lightrec CI assembles and tests the repository | missing | No application, Gradle/NDK composition, `android-port` consumption, touch layer or SAF setup here. |
| S023 | Tomba! 2 loads complete without loading-only waits or presentation | missing | No load operation classified yet; `pc_skip` still takes the title's loading branch (card `kanban-009`). |
| S024 | Tomba! 1 loads complete without loading-only waits or presentation | missing | No load operation classified for Tomba! 1. |
