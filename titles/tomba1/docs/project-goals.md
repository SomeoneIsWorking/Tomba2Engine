# Tomba! 1 project goals

These title-local goals refine the repository goals for Tomba! 1. Capability coverage belongs in
`project-state.md`, atomic work in `issues/`, and placement in `codemap.md`.

## G001 — Faithful, independently verified PC port

**Outcome.** Build a readable Tomba! 1 engine from the USA `SCUS_942.36` executable and disc content,
using the statically recompiled path as retained evidence and an independent reference engine as the
oracle.

**Why it matters.** A repository scaffold or matching executable hash is not a game. The title must
reach observable play through its own evidence-grounded engine.

**Success conditions.** The actual product provisions user-supplied assets, boots, renders, accepts
input, and reaches ordinary gameplay. Differential boundaries cover the executed path and prove both
agreement and a forced mismatch. Title startup, dispatch, game behavior, and generated ownership are
independent from Tomba! 2.

**Constraints and non-goals.** The implementation remains independent from the Tomba! 2 engine despite
sharing a repository. Generated output is never hand-edited or committed. Compilation, framework
smoke, and isolated tests alone do not satisfy this goal.

**Contributing state items.** S001, S002, S003, S004, S006.

## G002 — True widescreen

**Outcome.** Extend Tomba! 1's own projection, visibility, edge coverage, and 2D layout to wide aspect
ratios while preserving its faithful 4:3 path.

**Why it matters.** Widescreen is the selected presentation enhancement for this title and should
expose more correctly projected game content rather than a stretched or cropped 4:3 picture.

**Success conditions.** The running product demonstrates additional world coverage, correct visual
culling, and authored left/right/center UI anchoring at representative wide ratios, with a controlled
4:3 comparison.

**Constraints and non-goals.** Interpolation/lerp, temporal frame history, a title-native renderer,
native graphics producers, native depth, and 60fps/native-rendering options are outside the title
surface. Unsupported modes are absent rather than retained as disabled compatibility paths. The
shared psxport renderer remains the platform presentation owner.

**Contributing state items.** S005, S006.
