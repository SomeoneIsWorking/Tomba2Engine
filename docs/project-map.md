## Title coverage

| title | status |
|---|---|
| Tomba! | not started — title slot only; separate seam and ownership not yet derived |
| Tomba! 2 | current implementation; subsystem evidence remains below |

The repository-level `game/` is Tomba! 2-specific. Tomba! 1 does not share that game layer.

## Build, launch, and C++ policy

| surface | owner | verified behavior |
|---|---|---|
| default launcher | `run.sh` → `tools/run.py` | no arguments resolve the user's disc and launch `tomba2_port`; `--resume [recording.pad]` is preserved |
| build graph | `CMakeLists.txt`, `cmake/tomba2_port.cmake` | configure refuses non-Clang C++; build identity and `compile_commands.json` come from the real target |
| C++ verifier | `external/psxport/tools/check_cpp_style.py` via CTest | one shared implementation checks all first-party formatting, all compile-backed C++ TUs with clang-tidy, and 1,200-line/default shrink-only caps |

Tomba carries the authoritative tracked `.clang-format` and `.clang-tidy` profiles. The normal CTest
does not install a pre-commit hook and does not duplicate the verifier in this repo.

## Shared render ordering

Tomba! 2 submits native world faces through psxport's `runtime/recomp/render_queue.{h,cpp}`. Equal-key
opaque ties use `runtime/recomp/ot_lifo_depth.{h,cpp}`, which encodes the PSX `AddPrim` head-insertion
order as raster-distinct authored depths; `gpu_vk_next_distinct_3d_depth` owns the Vulkan `ord3d` mapping
needed to prove those depths remain distinct after conversion. Game code must not duplicate or
special-case that ordering policy.

Textured raster sampling is framework-owned. `external/psxport/runtime/recomp/shaders_gpu/psx_uv.glsl`
is the one integer-native-pixel UV-phase implementation shared by opaque, semi-transparent, and
semi-cover shaders. `gpu_vk_texture_phase_selftest.cpp` drives the shipping queue/shader/readback path
at 1x and 3x for positive/negative X/Y and mixed slopes; game producers must preserve guest packet UVs
instead of compensating for host sampling phase.

## Publication cleanliness audit (2026-07-23)

`git push` publishes the FULL HISTORY, so a machine path or copyrighted blob committed once is
expensive to remove later — it means rewriting history.

- **`tools/go_public.py`** — audits (A) copyrighted/oversized blobs, (B) machine-specific paths
  (`/home/<user>`, `~/`, usernames), (C) committed docs referencing private gitignored data.
  `scan --current` = working tree + HEAD (fast); `scan` = full history; `rules -o replace.txt`
  emits a `git filter-repo --replace-text` mapping when history does need scrubbing.
  Vendored into `tools/` deliberately so the audit travels with the repo.

**Remediation order when history is dirty: scrub, don't wipe.** `git filter-repo` rewrites hashes
but PRESERVES every commit and message; squashing to one orphan commit destroys them and is a last
resort only when scrubbing is genuinely intractable.

`tools/kanban.py` also refuses card text carrying machine-identifying data. That is not
hypothetical: a card body written with backticks inside a DOUBLE-quoted shell string had `` `w` ``
command-substituted, baking a real username/tty line into a committed file. Put long card bodies in
a file and pass `--body "$(cat f.txt)"`, or single-quote them.
