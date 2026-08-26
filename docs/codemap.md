# Repository codemap

This map owns subsystem placement for both titles. It does not report progress, goals, issues, or
verification status. Tomba! 2's generated guest-address-to-native-owner index remains the separate
specialized `docs/code-map.md`.

```text
CMakeLists.txt
├── cmake/tomba2_port.cmake              Tomba! 2 product composition
│   ├── game/                            Tomba! 2 engine only
│   └── generated/                       Tomba! 2 generated substrate only
├── titles/tomba1/cmake/tomba1_port.cmake
│   └── titles/tomba1/                   Tomba! 1 engine and evidence only
└── external/psxport/                    shared PSX platform/framework
```

## Ownership

| Subsystem | Responsibility | Current or target location | Entry point | Deep document |
|---|---|---|---|---|
| Repository composition | Compose title-specific CMake fragments without sharing game code | `CMakeLists.txt` | root configure | `docs/project-map.md` |
| Tomba! 2 application | Compose the process and own native-renderer plus temporal-presentation policy | `game/core/main.cpp`, `game/core/tomba_runtime.*`, `game/game_tomba2.cpp` | `main`, `TombaRuntime`, `tomba2_frame` | `docs/pc-game-architecture.md` |
| Tomba! 2 engine | Own Tomba! 2 gameplay, scene, objects, world, player, input, audio, and UI | `game/{core,scene,object,world,player,input,audio,ui}/` | `Game`, `Engine` | `docs/engine_re.md` |
| Tomba! 2 renderer | Build the picture from Tomba! 2 game state and submit native scene/UI passes | `game/render/` | `Render::renderScene` | `docs/render-arch.md` |
| Tomba! 2 interpolation | Own prior/current presentation state and intermediate-frame policy | `game/render/effect_lerp.*`, framework Fps60 seam through `game/render/fps60_worldpass.cpp` | `Fps60::frame_commit`, `EffectLerp::resolve` | `docs/fps60-rework.md` |
| Tomba! 2 widescreen | Own title projection, culling, world coverage, and 2D layout policy | `game/render/`, `game/scene/startup.cpp` | `gpu_vk_wide_engine`, native projection owners | `docs/wide60_recomp_widescreen.md` |
| Tomba! 2 generated substrate | Hold regenerable static-recompiler output for the Tomba! 2 executable/overlays | `generated/` | generated registries | `docs/port-framework.md` |
| Tomba! 2 address ownership index | Map guest addresses to native Tomba! 2 owners | `tools/codemap.py`, `docs/code-map.md` | `tools/codemap.py --addr` | `docs/code-map.md` |
| Tomba! 1 application | Compose a future Tomba! 1 runtime and product lifecycle | `titles/tomba1/game/app/` | future `game/app/main.cpp` | `titles/tomba1/game/app/README.md` |
| Tomba! 1 engine | Own Tomba! 1 executable, dispatch, game behavior, and title contracts | `titles/tomba1/game/core/` | future `Tomba1Runtime` | `titles/tomba1/docs/codemap.md` |
| Tomba! 1 widescreen | Own only executable-derived projection, visibility, edge coverage, and 2D layout | `titles/tomba1/game/render/` | future title-local projection/layout owners | `titles/tomba1/game/render/README.md` |
| Tomba! 1 generated substrate | Hold only regenerable Tomba! 1 output | `titles/tomba1/generated/` | future title-local generated registry | `titles/tomba1/docs/re-frontier.md` |
| Shared PSX platform | Own generic CPU/hardware, renderer backend, input/audio devices, configuration, and oracle infrastructure | `external/psxport/` | `psxport` library APIs | `external/psxport/AGENTS.md` |

## Where does new work go?

| Concern | Owner |
|---|---|
| A Tomba! 2 guest address, gameplay rule, object, scene, or native producer | root `game/` subsystem selected by responsibility |
| A Tomba! 1 guest address, gameplay rule, runtime, projection, or layout policy | corresponding `titles/tomba1/game/` subsystem |
| Generic PSX hardware, host renderer backend, physical input/audio device, or oracle mechanism | shared psxport repository |
| A Tomba! 2 player renderer or temporal-capability decision | `TombaRuntime`; shared psxport consumes its typed capability profile when filtering UI/config surfaces |
| Tomba! 2 address-level ownership lookup | `tools/codemap.py` and generated `docs/code-map.md` |
| Epic product intent | `docs/project-goals.md` or title-local goal document |
| Capability coverage | `docs/project-state.md` or title-local state document |
| Atomic work, bugs, findings, and dead ends | `docs/issues/` or title-local issue catalog |
