# Tomba2Engine

A **PC-native reimplementation of the *Tomba! 2: The Evil Swine Return* engine** in C++.

The goal is to rebuild Tomba! 2 as a self-contained, PC-native game engine that runs the **real
game content** — not an emulator, and not a recompiled-MIPS blob with I/O bolted on. The engine
owns the world, objects, camera, projection, and rendering; it behaves like a PC game rather than
simulating the PlayStation.

It is built on [**psxport**](https://github.com/SomeoneIsWorking/psxport) (vendored as
`external/psxport`), a game-agnostic PSX static-recompilation + native-hybrid framework: a MIPS→C
recompiler, a PSX platform layer (GPU/SPU/GTE/MDEC/CD backends), a side-by-side differential-compare
harness, and a Vulkan renderer.

> **You must supply your own disc image.** No game assets, ROMs, or disc images are included or
> distributed. You need a CHD of a Tomba! 2 disc you legally own.

---

## How it works (the short version)

Any run is one **execution path** × one **rendering path**:

- **Execution** — the ported game logic runs either on the **recomp substrate** (the statically
  recompiled `MAIN.EXE`, used as the byte-exact reference) or on the **PC-native engine** (clean OOP
  C++ classes that reproduce the substrate's behavior). Native ownership is grown function by
  function and gated for byte-exactness against the substrate.
- **Rendering** — either the **PSX renderer** (the substrate's GTE + ordering-table + GP0 path, used
  as a visual reference) or the **native renderer** (`pc_render`), which draws the picture from the
  game's own object data with real per-pixel depth — no GTE/OT/GP0 transcription.

The full vocabulary and the project's working rules live in [`CLAUDE.md`](CLAUDE.md).

---

## Requirements

Install once:

- **Linux:** `cmake`, `pkg-config`, `SDL3` (`SDL3-devel` / `libsdl3-dev`), `libzstd`, `zlib`,
  `python3`, and a C/C++ toolchain (`build-essential`).
- **macOS:** `brew install cmake pkg-config sdl3 zstd zlib python3`
- A **Vulkan-capable GPU + drivers** (the renderer is Vulkan).

The GTE/MDEC/SPU/CHD hardware backend comes from a vendored `beetle-psx` fork (a nested submodule of
psxport) — so clone recursively.

---

## Getting started

```bash
# 1. Clone with submodules (psxport + its nested beetle-psx backend)
git clone --recursive https://github.com/SomeoneIsWorking/Tomba2Engine.git
cd Tomba2Engine

# 2. Provide your disc image (any ONE of these):
#    - pass it on the command line, or
#    - copy .env.example to .env and set the path, or
#    - drop a *.chd file in the repo root
cp .env.example .env      # then edit PSXPORT_TOMBA2_DISC

# 3. Build + run (does everything end to end)
./run.sh                  # or: ./run.sh /path/to/Tomba2.chd
```

`run.sh` builds the CHD tooling, extracts `MAIN.EXE` from your disc, recompiles the game core + native
runtime, and launches the game in a window.

**Disc resolution order** (everywhere): CLI arg → `PSXPORT_TOMBA2_DISC` → `.env` → a `*.chd` drop-in
in the repo root.

**Useful environment knobs:**

| Var | Effect |
|-----|--------|
| `PSXPORT_NOWINDOW=1` | headless run (no window) |
| `PSXPORT_NOAUDIO=1` | mute |
| `PSXPORT_GPU_DUMP=<dir>` | dump frames as PPM |
| `PSXPORT_DEBUG=<chan,chan>` | enable diagnostic channels (see `docs/config.md`) |
| `CC=clang` / `CC=gcc` | override the compiler |

### Building without running

```bash
cmake -S . -B build                              # configure once
cmake --build build --target tomba2_port         # rebuild only
# binary: scratch/bin/tomba2_port
```

---

## Project structure

```
game/            PC-native game engine, organized by subsystem:
  ai/            enemy/actor behaviors, state machines
  object/        object table, animation, behavior dispatch, script VM
  world/         entity tables, placement, spawning, pools
  player/        Tomba actions, collision, hitboxes
  camera/        scene + cutscene cameras
  render/        native renderer (pc_render): producers, projection, culling, submit
  scene/         scene/level load, transitions, script interpreter
  audio/         SFX, BGM sequencer, XA streaming
  input/  items/  ui/  math/  core/
  game_tomba2.cpp, tomba2_types.h   (top-level game entry + shared types)
external/psxport/  the framework submodule: recompiler, PSX platform, SBS harness, VK renderer
generated/       recompiled MAIN.EXE shards (git-ignored, regenerated from your disc)
tools/           the recompiler + the tracking-stack tooling (see below)
docs/            architecture, RE notes, per-subsystem status, findings
replays/         deterministic pad-capture replays for reproducing scenarios headlessly
scratch/         git-ignored run artifacts (binary, logs, screenshots)
```

`scratch/bin/tomba2_port` **is the game**: the recompiled `MAIN.EXE` (`generated/shard_*.c`) linked
with the native game (`game/*`) and the PSX platform (`external/psxport/runtime/*`). The CMake source
list lives in `cmake/tomba2_port.cmake`.

---

## Contributing

This is a reverse-engineering + reimplementation project, so the workflow is disciplined. The full,
authoritative rules are in [`CLAUDE.md`](CLAUDE.md); the essentials:

**Reverse-engineer first — never black-box.** Before reimplementing a function, decompile and
understand it (Ghidra headless via `tools/decomp.sh`). Reproduce the observable *result*, not the PSX
mechanism — no magic constants, no packet transcription, no GTE/OT/GP0 inheritance.

**No bandaids.** Fix the root cause, not the symptom. A change that hides a symptom without explaining
why it happened is not a fix. Stopgaps must be named as such and justified.

**Grow native ownership.** Enemy AI, physics, quests, camera, rendering — all portable to native C++
classes. Un-ported code runs as the recompiled substrate. Real OOP classes with headers and state on
the owning subsystem — no free-function shims, no file-scope globals.

**The tracking stack** — consult at the start of a task, update in the same commit as the work:

| Tool | Question it answers |
|------|--------------------|
| `tools/codemap.py --addr <hex>` | *Where* is a function — who owns guest address X? |
| `tools/portmap.py next` | *What* to port next (the RE dependency chain) |
| `tools/parity.py` | Is a unit byte-exact vs the substrate? |
| `tools/behavior.py` | What deliberately diverges from the reference? |
| `tools/findings.py <symptom>` | Has this bug been seen before? (search first) |

**Verification.** Execution work is gated for **byte-exactness** against the recomp substrate via the
side-by-side (SBS) harness — a divergence is a bug. Prefer static equivalence checks and short smokes;
reserve long SBS runs for real regressions. Rendering work is verified by **observing the result** (the
picture) — build, run, and eyeball the scene; the native renderer is read-only and must never write
guest memory.

**Build discipline.** One `main` branch, commit directly. Keep committed files portable — no
machine-specific paths, and never commit disc images or other copyrighted assets.

See `docs/` for architecture and status:
[`port-progress.md`](docs/port-progress.md) (the boot→gameplay spine),
[`render-arch.md`](docs/render-arch.md), [`engine_re.md`](docs/engine_re.md),
[`driving-the-game.md`](docs/driving-the-game.md), and [`config.md`](docs/config.md).

---

## License

The engine/port code in this repository is provided as-is for research and preservation. It contains
**no game assets** — Tomba! 2 and its content are the property of their respective rights holders; you
must supply your own legally-obtained disc image to run it. The vendored `beetle-psx` backend is
GPL-2.0 (see `external/psxport/runtime/vendor/beetle-psx`).
