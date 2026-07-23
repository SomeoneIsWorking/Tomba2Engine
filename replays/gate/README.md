# SBS coverage-gate routes (`*.sbskeys`)

A `.sbskeys` file is a `PSXPORT_SBS_KEYS` string: comma-separated `from-to:button` frame
ranges, mirrored to BOTH SBS cores. It exists to solve the coverage problem in
docs/findings/sbs.md — a boot-only SBS run reaches only ~236/411 owned addresses (~57%),
so ~43% of the port is never byte-compared. A driven route walks the gate into gameplay,
where the interesting natives run.

**Why a KEYS string and not a `.pad`:** SBS core A is hard-wired `pc_skip=false`, but every
`./run.sh` capture in `replays/*.pad` is `pc_skip=true`. The two boot cadences differ, so a
frame-indexed `.pad` lands its inputs at the wrong moments under SBS and does NOT reproduce
the route (it can lower coverage — measured 230 < 236). A KEYS route is authored against
SBS's own frame axis, so it is valid by construction. See docs/findings/sbs.md
"COVERAGE-limited".

## Run a gate route

    PSXPORT_SBS_KEYS="$(cat replays/gate/seaside-sweep.sbskeys)" \
    PSXPORT_SBS_EXIT_FRAME=27500 \
    PSXPORT_SBS_MODE=full PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 \
    ./scratch/bin/tomba2_port </dev/null

Read the `coverage:` line it prints at exit and the divergence count. A divergence in the
driven span is a REAL bug that the boot-only gate could never have seen.

## Routes

- **seaside-sweep.sbskeys** — from cold boot: tap Cross every 55 frames to blow through the
  long opening cutscene (title-select ~f1650 through the "house is on fire" dialogs to
  ~f17000), then a free-roam program in the area-0 seaside start (walk all four directions,
  jump, grab, open the item menu). Reaches **273/411 owned addresses (66%), 0 divergences**
  through f27500 (verified 2026-07-23). The residual ~138 are dominated by other-area /
  event-triggered code (61 in field_owned_leaves.cpp, plus enemy/object behaviours for
  actors not present in the seaside start) — those need REAL traversal of the game, not a
  denser blind route. Extend this file (or add a new route that reaches another area) to
  push coverage further.

## Building a route

Author frame ranges against SBS's `mFrame` axis (roughly 10x the REPL `run` counter). Take
`PSXPORT_SBS_SHOT="<f0>,<f1>:scratch/screenshots/route"` snapshots to see where the run is,
then place input ranges accordingly. The intro is input-gated (dialogs advance on Cross), so
a route must tap through it before any walking has effect. The `PSXPORT_SBS_KEYS` buffer is
64 KB, so routes can be long.
