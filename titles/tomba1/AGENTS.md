# Tomba! 1 title authority

This subtree owns the Tomba! (`SCUS_942.36`, USA) port. It shares a repository and the generic
`psxport` framework with Tomba! 2, but it is a separate engine implementation.

## Non-negotiable boundary

- Tomba! 1 source lives below `titles/tomba1/game/`. It must not compile or include the repository's
  top-level `game/` tree, which is the Tomba! 2 engine.
- Tomba! 1 owns a separate `GameRuntime`, recompiled-substrate registry, executable facts, seed set,
  tests, tools, and generated output (`titles/tomba1/generated/`). Never reuse a Tomba! 2 guest address,
  overlay assumption, hook table, renderer policy, or compatibility adapter.
- The process entry point belongs at `game/app/main.cpp` and only composes owners. Runtime behavior
  belongs in `game/core/`; widescreen projection and layout belong in `game/render/`.
- `titles/tomba1/generated/` is regenerable and sacrosanct. Never edit or commit generated code.
- Disc images and extracted executables remain untracked. The future provisioner must resolve CLI
  argument > `PSXPORT_TOMBA1_DISC` > `.env` > one repository-root CHD.

## Enhancement scope

Tomba! 1's enhancement target is widescreen only. Do not add interpolation/lerp, temporal frame
history, a title-native renderer, native graphics producers, or native-depth work. Do not expose
60fps or native-rendering configuration options: neither capability exists in this title's scope, so
an off-by-default option would still be a false compatibility path. `enhancement_scope.json` contains
only the positive widescreen capability; absence is the machine-readable contract enforced by
`tools/verify_title_isolation.py`. Widescreen starts only after
executable-grounded RE identifies the game's projection, visibility culling, and 2D layout owners. A
host stretch, OT/GP0 reconstruction, or GTE-output-derived picture is not widescreen.

## Working loop

Start by consulting the title-local registries from this directory:

```sh
python3 ../../tools/info.py brief <terms>
```

Read `docs/re-frontier.md` before choosing an RE step and `docs/codemap.md` before placing code. Update
the frontier, project state, claims/instruments, and codemap in the same change that moves their
respective facts or ownership.

Current non-running checks:

```sh
python3 tools/verify_executable.py --selftest
python3 tools/verify_title_isolation.py
```

When a provisioned executable exists, verify the real bytes explicitly:

```sh
python3 tools/verify_executable.py --check scratch/bin/tomba1/SCUS_942.36
```

The repository root includes the title CMake fragment so its evidence and isolation checks run with
the combined tree. Until the first oracle boundary exists, it exposes only the framework library, identity,
and isolation checks; it must not publish a fake `tomba1_port` executable.
