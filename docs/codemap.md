# Repository codemap

This map owns placement for both titles. It does not report progress, goals, issues, or evidence.
Execution order belongs in `docs/migration.md`; capability state in `docs/project-state.md`.
Tomba! 2's specialized address-to-native-owner index remains `docs/code-map.md`.

```text
CMakeLists.txt
├── cmake/tomba2_port.cmake              Tomba! 2 product composition
│   └── game/                            Tomba! 2 native title owners
├── titles/tomba1/cmake/tomba1_port.cmake
│   └── titles/tomba1/game/              Tomba! 1 native title owners
└── external/psxport/                    shared Lightrec executor and PSX platform
```

## Ownership

| Subsystem | Responsibility | Current or target location | Entry point | Deep document |
|---|---|---|---|---|
| Repository composition | Compose isolated title products around one shared runtime without sharing game code | `CMakeLists.txt`, `cmake/`, title-local CMake fragments | root configure | `docs/migration.md` |
| Shared PSX executor | Translate remaining guest instructions with Lightrec; own per-`Core` synchronization, bounded exits, runtime image generations, override/original-call dispatch, and invalidation | `external/psxport/` | shared executor API | psxport's own codemap and migration docs |
| Shared PSX services | Own generic CPU/device state, HLE callbacks, CD/GPU/SPU/MDEC services, physical input/audio, renderer backend, and independent test infrastructure | `external/psxport/` | `psxport` library APIs | psxport docs |
| Tomba! 2 application | Compose the process and one finite title frame transaction; own title-aware auto-drive, diagnostics, REPL commands, renderer capability, and temporal policy | `game/core/main.cpp`, `game/core/tomba_runtime.*`, `game/core/frame_driver.*`, `game/core/auto_drive.*`, `game/core/frame_diagnostics.*`, `game/core/repl_commands.cpp`, `game/game_tomba2.cpp` | `main`, `TombaRuntime::createFrameDriver`, `TombaFrameDriver::stepFrame` | `docs/pc-game-architecture.md` |
| Tomba! 2 runtime overrides | Own deliberately native game behavior and bind resident declarations to the explicit resident image token and text range at boot load | smallest responsibility owner under `game/`; declaration catalog in `game/core/native_override_catalog.*` | `TombaRuntime::registerOverrides`, final boot binding in `TombaRuntime::bootInit` | `docs/migration.md`, `docs/code-map.md` |
| Tomba! 2 guest continuation | Adapt title-native resume requests to the current PSXPort dynamic executor without exposing executor internals across game subsystems | `game/core/guest_resume.h` | `tomba::requestGuestContinuation` | `docs/migration.md` |
| Tomba! 2 engine | Own gameplay, scene, objects, world, player, input, audio, UI, items, camera, CD, and math policy | `game/core/`, `game/scene/`, `game/object/`, `game/world/`, `game/player/`, `game/input/`, `game/audio/`, `game/ui/`, `game/items/`, `game/camera/`, `game/cd/`, `game/math/`, `game/ai/` | `Game`, `Engine` | `docs/engine_re.md` |
| Tomba! 2 renderer | Build the picture from Tomba! 2 game state and submit native scene/UI passes | `game/render/` | `Render::renderScene` | `docs/render-arch.md` |
| Tomba! 2 interpolation | Own prior/current presentation state and intermediate-frame policy | `game/render/effect_lerp.*`, `game/render/fps60_worldpass.cpp` | `Fps60::frame_commit`, `EffectLerp::resolve` | `docs/fps60-rework.md` |
| Tomba! 2 widescreen | Own title projection, culling, additional world coverage, and scene-specific 2D composition | `game/render/`, including `title_wide_composition.cpp`, plus `game/scene/startup.cpp` | `gpu_vk_wide_engine`, `Render::titleWideMargins`, native projection owners | `docs/render-arch.md` |
| Tomba! 2 address ownership index | Map guest image/address identities to native Tomba! 2 owners | `tools/codemap.py`, `docs/code-map.md` | `tools/codemap.py --addr` | `docs/code-map.md` |
| Tomba! 2 retired-execution exclusion | Keep the removed generator, emitted guest source, and offline dispatcher absent; prohibit interpreter-first gameplay modes | absent by design; enforced by CMake, provisioning, and policy checks | no entry point | `docs/migration.md` |
| Repository tests | Own title behavior, structure, launcher, production-boundary checks, and fallback telemetry thresholds | `tests/` and title-local test directories | explicit test executables and Python tests | test source nearest the production boundary |
| Repository tooling | Own title provisioning, documentation checks, RE inspection, and maintenance without owning product execution | `tools/` and title-local tool directories | cohesive Python entry point | tool `--help` and nearest workflow doc |
| Hosted verification | Supply title targets and paths to PSXPort's shared consumer configure/build/test/binary verifier | `.github/workflows/ci.yml`, `tools/verify_ci.py`; shared sequence in `external/psxport/tools/port/consumer_verify.py` | `tools/verify_ci.py` | `docs/project-state.md` |
| Tomba! 1 application | Compose only Tomba! 1 process-lifetime owners around the shared executor | `titles/tomba1/game/app/` | `titles/tomba1/game/app/main.cpp` | `titles/tomba1/game/app/README.md` |
| Tomba! 1 runtime overrides | Own Tomba! 1 startup, frame, CD/DMA, and deliberately native behavior by complete image identity | `titles/tomba1/game/core/` | `Tomba1Runtime` and title override composition | `titles/tomba1/docs/codemap.md` |
| Tomba! 1 widescreen | Own only executable-derived projection, visibility, edge coverage, and 2D layout | `titles/tomba1/game/render/` | future title-local projection/layout owners | `titles/tomba1/game/render/README.md` |
| Interpreter policy | Keep forced interpretation diagnostic-only; bounded automatic fallback and reason telemetry are owned once by psxport | shared `external/psxport/` | shared executor policy API | psxport execution documentation |

## Where does new work go?

| Concern | Owner |
|---|---|
| Lightrec decode/emission, CPU state synchronization, bounded exits, image generations, code invalidation, or generic override/original-call dispatch | shared psxport |
| A Tomba! 2 guest address, gameplay rule, object, scene, native producer, or override selection | root `game/` subsystem selected by responsibility |
| Tomba! 2 per-frame input/timing/scheduler/render order or presentation boundary | `game/core/frame_driver.*`; shared psxport only repeats the finite transaction |
| Tomba! 2 stage-aware automation, guest-layout probes, or title-specific REPL inspection | `game/core/auto_drive.*`, `game/core/frame_diagnostics.*`, `game/core/repl_commands.cpp` |
| A Tomba! 1 guest address, runtime, gameplay rule, projection, or layout policy | corresponding `titles/tomba1/game/` subsystem |
| Offline guest-code emission, generated registries, or interpreter-first gameplay selection | nowhere; these have no target owner |
| Asset-free hosted build/test orchestration | `.github/workflows/ci.yml` calling `tools/verify_ci.py`; platform support is recorded independently in project state |
| Epic intent | `docs/project-goals.md` or the title-local goals document |
| Capability coverage | `docs/project-state.md` or the title-local state document |
| Atomic work and evidence | `docs/issues/`, `docs/info/`, or title-local equivalents |
