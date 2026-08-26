# Project goals

This repository hosts two Tomba ports. They share the repository and psxport platform, but not a
game engine. Factual capability coverage belongs in `project-state.md`, atomic work in `issues/`,
and ownership/placement in `codemap.md`.

## G001 — Playable Tomba! 2 PC-native engine

**Outcome.** Tomba! 2 runs from user-supplied game content as a self-contained PC-native game engine
with faithful gameplay, input, audio, saves, movies, menus, and transitions.

**Why it matters.** Executing recompiled MIPS is a useful retained substrate, not the final product;
the maintained default path must be a usable game whose behavior is owned by readable native
subsystems.

**Success conditions.** `./run.sh` launches the intended Tomba! 2 product from a fresh clone; ordinary
play can progress through the authored game; native ownership preserves the independently observed
game contract; unsupported or missing behavior refuses by name rather than silently substituting a
smaller path.

**Constraints and non-goals.** Generated code remains regenerable and unedited. Restricted assets
remain user supplied. The recompiled path remains available as differential evidence, not as a second
product whose behavior is allowed to drift.

**Contributing state items.** S001, S002, S003, S007.

## G002 — Tomba! 2 native wide and interpolated presentation

**Outcome.** Tomba! 2 renders from game-owned scene state through its native renderer, supports true
widescreen, and presents correctly interpolated frames between simulation ticks.

**Why it matters.** Native scene ownership is the common prerequisite for a wider view, real depth,
and stable interpolation without reconstructing meaning from GTE, ordering-table, or GP0 output.

**Success conditions.** Every visible layer has an attributed game-state producer; widescreen shows
additional correctly projected content with correct culling and anchored 2D layout; interpolation
uses stable previous/current camera, object, and effect state through the same render path as real
frames; representative real-game runs and controlled oracle legs establish the picture and cadence.

**Constraints and non-goals.** Render code does not write guest memory. Guest packets and GTE output
are diagnostic evidence, never picture inputs. Missing producers remain explicit rather than gaining
a family-wide fallback or renderer special case.

**Contributing state items.** S004, S005, S006.

## G003 — Playable, engine-isolated Tomba! 1 port

**Outcome.** Tomba! runs from user-supplied `SCUS_942.36` content through its own executable-grounded
runtime and game-engine implementation inside this repository.

**Why it matters.** Co-locating two titles is organizational convenience only. Their incompatible
engines must stay independently understandable and verifiable.

**Success conditions.** Tomba! 1 has its own provisioner, generated registry, runtime, product target,
tests, and independent oracle evidence; the actual product boots, renders, accepts input, and reaches
playable game content; no Tomba! 2 guest address, game source, renderer assumption, or compatibility
adapter crosses the title boundary.

**Constraints and non-goals.** Tomba! 1 code lives under `titles/tomba1/`; the root `game/` and
`generated/` trees remain Tomba! 2-only. No fake product or launcher is published before an
executable-backed execution path exists.

**Contributing state items.** S008, S009, S010, S012.

## G004 — Tomba! 1 true widescreen without unrelated enhancement modes

**Outcome.** Tomba! 1 supports true widescreen through its own projection, visibility, edge-coverage,
and 2D-layout owners.

**Why it matters.** A wider view is the selected presentation enhancement for this title; unrelated
renderer and cadence systems would add compatibility paths without serving that outcome.

**Success conditions.** The running product preserves its faithful 4:3 path and displays additional
correctly projected world content at wide aspect ratios; visual culling and authored UI anchors are
handled explicitly; the title exposes no interpolation, temporal-history, title-native-renderer,
native-producer, native-depth, 60fps, or native-rendering option.

**Constraints and non-goals.** Widescreen is not host stretching, cropping, or reconstruction from
post-projection data. The shared psxport renderer remains the platform presentation owner.

**Contributing state items.** S011, S013.
