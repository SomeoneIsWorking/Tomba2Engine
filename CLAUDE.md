# Tomba Engine repository authority

**Unlabeled content is machine convention, revisable by any session. USER lines are verbatim dated
quotes and only those.**

> USER, 2026-06-14: *"new direction — port to PC, no PSX emulation, no PSX BIOS."*
>
> USER, 2026-06-14: *"make the game itself do PC native rendering instead of PSX emulated rendering."*

## Product direction

This repository contains two isolated native/dynarec ports. Title-owned C++ subsystems and overrides
implement deliberately native behavior. Every remaining guest instruction executes on demand from the
user's authenticated executable or active overlay through the shared `psxport` Lightrec dynarec.

The gameplay products contain no offline-emitted guest C/C++, object corpus, or precompiled title
substrate. They do not link or select an interpreter and contain no fallback to one. An interpreter
may exist only in a separately built test target, including diagnostics, for deterministic diagnosis
against the same canonical CPU, memory, exception, and device boundary. Product proof requires link
and selector inspection, not merely observing zero interpreter entries in one run.

`docs/migration.md` is the execution-plan authority. Both static products and their generation
machinery are already removed; do not recreate them. Retain measured behavioral and binary evidence,
but no compatibility mode or static oracle.

## Title order and isolation

Tomba! 2 (`SCUS_944.54`) is first. Root `game/` is its title engine. It must prove one resident
native override and a colliding-overlay override plus scoped original calls through Lightrec, restore
the recorded gameplay frontier, and pass representative gameplay.

Tomba! 1 (`SCUS_942.36`) follows and lives entirely below `titles/tomba1/`. It reuses only the
generic `psxport` executor and services. Never reuse a Tomba! 2 address, overlay assumption, game
owner, renderer policy, or compatibility adapter. Its first dynarec boundary is the recorded 35-field
CRT0 comparison; it then crosses the current CD/movie boundary, reaches the title screen, demonstrates
input, and passes representative gameplay.

## Shared and title ownership

- `external/psxport/` owns Lightrec integration, per-`Core` CPU synchronization, bounded exits,
  runtime image generations, executable-memory invalidation, generic native-override/original-call
  dispatch, PSX hardware/HLE services, host devices, and independent test infrastructure.
- Each title owns executable/disc identity, native game behavior, title policy, frame composition,
  rendering policy, and enhancement semantics.
- Lightrec owns its translated-code memory and cache. Do not wrap or duplicate those owners locally.
- Native override keys include complete resident/overlay image generation plus guest address. Address
  alone is invalid where overlays reuse a slot.
- A normal call observes the active native override. A scoped original call suppresses only that
  override for the duration of one call and executes the matching guest body through Lightrec.
- Loading or replacing executable bytes and changing an override invalidates every translated path
  that captured the prior bytes or dispatch decision.
- Host work, VSync/frame suspension, interrupts, exceptions, and thread completion use explicit
  bounded executor exits. Do not unwind C++ exceptions through translated frames.

## Preserved execution facts

Tomba! 2's `game/core/frame_driver.*` is the sole per-frame transaction owner.
`TombaRuntime::bootInit` is finite. One `TombaFrameDriver::stepFrame` composes title-aware input,
timing/event delivery, game tasks, native rendering, diagnostics, and exactly one presentation fence.
Guest VSync at `0x80085900` is fatal; a successful guest wait/query or timeout is never pacing.

Recorded pre-migration runs reached GAME at frame 25, free-roam at frame 216, and 620/620 presentation
fences. These are the frontier to regain, not Lightrec evidence. Binary findings identify
`0x801113B4` in both A03 and A0B with different entry shapes, providing a grounded image-collision
case for runtime dispatch and invalidation.

Tomba! 1's preserved facts are title-local. Its CRT0 contract includes BSS
`[0x8009AFB0,0x800A3348)`, SP `0x801FFFF8`, heap `0x800A3348+0x15C8B0`, gp
`0x80097FA8`, first `A(39h)` wrapper `0x8006B70C`, and game main `0x800163B0`.
The deterministic boundary agrees on 35/35 target/PC/register fields and has forced-mismatch plus
too-short negatives. The current CD facts include public/internal `CdSync` at `0x800648C8` and
`0x80065470`, fatal guest-VSync return address `0x800654A4`, DMA registration `0x80067E84`,
callback `0x80066D80`, and recorded movie progress beyond LBA 58739.

## Native engine rules

- Native ownership is a deliberate title boundary, not a workaround for missing CPU semantics.
  Fix MIPS decode/lowering/state defects in shared Lightrec integration, never at one guest address.
- RE before implementation. Use Ghidra headless and the executable or authenticated overlay to
  understand a behavior. Generated output may corroborate a historical finding but is not the
  implementation recipe or a source of new executable bodies.
- Native code reads as game code: cohesive classes, typed views over guest-visible state, explicit
  invariants, named states/constants, narrow interfaces, and no opaque address soup.
- Use the shared guest ABI boundary for arguments, results, and guest-visible state. Preserve required
  guest stack/register behavior when native and guest code interoperate, but do not transcribe an
  instruction stream into C++.
- One behavior has one owner. Do not add environment-gated duplicate implementations, raw fallback
  tables, title-local CPU/cache wrappers, or special cases for one failing input.
- Fail fast on missing execution, invalid identity, unknown service boundaries, and impossible state.
  Do not swallow, retry-until-pass, fast-forward, or patch phase/timer/scene state.
- Title launchers and automation are Python through the locked `uv` environment. `run.sh` remains
  the only shell entry point and never runs tests.

## Rendering

Tomba! 2's picture comes from game-owned scene state. The native renderer owns world/object/camera
projection, native depth for 3D, and explicit layer policy for 2D. It must not derive a shipping
picture from GTE output, ordering-table entries, GP0 packets, adjacent frames, or content-dependent
pixel sampling. Rendering and interpolation write no guest memory.

Do not restore guest-OT fallback, packet stamping/tagging, output reconstruction, or final-image
stretching. Missing pictures require a native producer reading the owning game state. Diagnostics may
inspect guest output to answer questions but never feed presentation.

The measured reason is retained: reconstructing transforms from composed GP0/OT output left camera-
dependent residue of 0.13 px camera-still and 1.53 px while panning, with 12/12 sign alternations,
net drift -1.23 px, and X/Y movement 1.53/0.43 px. Resolve transforms from their game-state submitter.

Tomba! 2 interpolation operates on matching native camera/object/effect state with explicit
provenance, through the same renderer as real frames. It is independent from widescreen coverage.

Tomba! 1 gets widescreen only. It must not add Tomba! 2's native renderer, producers, depth,
interpolation, temporal history, or 60fps/native-rendering options. Widescreen begins after gameplay
conformance and changes projection, viewport/draw-buffer, proven visual culling, and 2D anchors to
show additional world content. Stretching or cropping does not qualify.

## Evidence and workflow

At task start run one information brief and consult the appropriate frontier:

```sh
uv run --frozen python tools/info.py brief <terms>
```

- `docs/project-goals.md`: durable outcomes and success conditions.
- `docs/project-state.md`: verified/partial/blocked/missing capability inventory and current focus.
- `docs/migration.md`: title order and migration/deletion gates.
- `docs/re-frontier.md` and `titles/tomba1/docs/re-frontier.md`: ordered RE dependencies.
- `docs/codemap.md`: subsystem placement. `docs/code-map.md` is the specialized Tomba! 2 address
  ownership index.
- `docs/issues/`, title-local issues, `docs/findings/`, and `docs/info/`: atomic work and evidence.
- `docs/unported-render-inventory.md`: missing Tomba! 2 picture producers.
- `docs/areas.md`: grounded area indices and reachability.

A diagnostic must report how much it scanned and prove both positive and negative answers. Absence of
a symptom is not evidence without reachability. Validate tools against a case that must differ.

Boot, logos, menus, attract loops, FMV, a leaf override, clean internal traces, and still screenshots
are checkpoints, not representative gameplay. The title completion gate covers player input, guest
PC/register and memory state, interrupts/timing, relevant CD/GPU/SPU behavior, native override
reachability, overlay invalidation, correctness, and frame-time budgets on each released host.
Independent evidence comes from a trusted emulator, hardware, binary analysis, or separately built
test interpreter in a separately built test target, including diagnostics—not the retired static
product. Link and selector inspection prove that interpreter is absent from gameplay products.

Preserve useful behavioral and address findings when replacing stale plans. Remove generated-symbol
workflows and static-process instructions rather than appending a contradictory dynamic section.
