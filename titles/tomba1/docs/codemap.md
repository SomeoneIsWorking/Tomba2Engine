# Tomba! 1 codemap

Tomba! 1 is a title-local consumer of the shared psxport framework. The repository-level `game/` tree
is another engine and is outside this map.

```text
game/app        composes process-lifetime owners
    |
game/core       owns executable/runtime/dispatch semantics
    |
psxport         owns generic PSX platform, hardware, presentation, and oracle infrastructure
    |
game/render     owns only title-derived widescreen projection and layout policy
```

## Ownership

| Subsystem | Responsibility | Current or intended location | Entry point | Deep document |
|---|---|---|---|---|
| Application | Compose runtime, registry, shared machine, and lifecycle | `game/app/` | future `game/app/main.cpp` | `game/app/README.md` |
| Executable identity | Compare the selected image with tracked whole-file and PS-X header facts | `executable.json`, `tools/verify_executable.py` | `verify_executable.py --check` | `docs/info/claims/001-executable-identity.md` |
| Framework seam | Own Tomba! 1 runtime policy and immutable guest facts | `game/core/` | future `Tomba1Runtime` | `game/core/README.md` |
| Generated registry | Bind only `titles/tomba1/generated/` symbols to psxport | `game/core/` | future `tomba1_install_recomp` | `game/core/README.md` |
| Harness integration | Expose the framework library, identity, isolation, and later oracle targets | `cmake/tomba1_port.cmake`, `tests/` | `tomba1_verify_scaffold` | `docs/re-frontier.md` |
| Widescreen projection | Publish the title-derived horizontal projection and guest-wide contract | `game/render/` | future `WidescreenProjection` | `game/render/README.md` |
| Widescreen layout | Preserve authored 2D anchors across a wide presentation | `game/render/` | future `WidescreenLayout` | `game/render/README.md` |
| Generated output | Hold regenerable static-recompiler shards | `titles/tomba1/generated/` | future emitter target | `docs/re-frontier.md` |

## Where does new work go?

| Concern | Owner |
|---|---|
| Process startup and shutdown ordering | `game/app/` |
| Guest addresses, ABI, boot, dispatch, and native behavior | `game/core/` |
| Projection, visibility, edge coverage, and HUD/menu anchoring | `game/render/` |
| Generic PSX hardware, renderer backend, input devices, audio devices, or differential engine | shared psxport repository |
| Epic intent, capability state, atomic work, and evidence | their respective `docs/project-goals.md`, `docs/project-state.md`, `docs/issues/`, `docs/info/`, and `docs/re-frontier.md` authorities; not this codemap |
