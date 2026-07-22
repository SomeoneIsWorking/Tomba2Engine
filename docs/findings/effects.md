# Effects — spawn/render findings

## [effects] Movement DUST CLOUDS — spawn/render RE (kanban #39)

**Symptom (USER 2026-07-23):** Tomba should kick up dust clouds on movement actions (starting to
walk, turning while running, others). Reported missing under pc_render AND "under the oracle."

**Status:** ROOT-CAUSED. The task's central premise — "absent on the oracle too, so not a render
gap" — is **FALSIFIED by direct measurement.** The dust effect DOES spawn and DOES render on the
oracle / psx_render. It is missing ONLY under pc_render, and it is the **same missing-native-producer
class as #12/#13/#21** (torch flame / HUD carousel / charge effect), not an emitter/spawn failure.

### How the premise was falsified (evidence)
- Instrumented the dust-spawn decision `FUN_80053670` and pool-spawn `FUN_800534B0` with a temporary
  logging override (removed after). Under the `replays/bugs/seesaw-weight.pad` replay the emitter
  FIRES: `decision obj=800e7e80 state=6 p2=0 p3=0 mat=0` → `SPAWN owner=800e7e80 subtype=1` (Tomba
  object 0x800E7E80, motion state 6 = walk-start, surface material 0).
- Captured the spawn frame (~seesaw-replay frame 310-320) under three configs:
  - `PSXPORT_RENDER_PSX=1` (psx_render): white dust puff clearly visible above Tomba
    (`scratch/screenshots/dust_psx_310.png`).
  - `PSXPORT_ORACLE=1` (pure recomp + psx_render, the true oracle): puff clearly visible left of
    Tomba at frame ~320 (`scratch/screenshots/orsweep_320.png`).
  - default (pc_render): NO puff (`scratch/screenshots/dust_pc_310.png`).
- Conclusion: dust is PRESENT on the oracle, ABSENT on pc_render → a render gap, contra the framing.

### The dust effect family (all UNOWNED substrate)
- **Spawn decision:** `FUN_80053670(actor, p2, p3)` — switch on surface material `actor+0x176`;
  spawns for materials 0-9 (default case >9 = no dust). `p3 == 8` means material-refresh only (NO
  spawn). Calls `FUN_800534B0` (pool alloc via `FUN_8007AB20`) to create the dust controller object.
- **Subtype** = `FUN_800535D4(actor)` = `actor[0x176] + 1` (surface material + 1) — dust appearance
  is per-surface (grass/sand/stone differ).
- **Dust-kind byte** `actor+0x146`: 1 = walk-start puff, 2 = run/skid puff, 3 = material-refresh (no
  spawn), 0 = neutral. `p3`: 0 = spawn at foot Y, 1 = spawn elevated (foot Y − 0x800, mid-turn), 8 = none.
- **Controller handlers** (jump table @ 0x800A4480, indexed by subtype−1): `FUN_8006AE28` ×3,
  `FUN_8006BB6C` ×3, `FUN_8006C478` ×4, `FUN_80069300`. Their state-entry helpers
  `FUN_80069BEC/8006B124/8006BDF0` play footstep SFX 0x22 and (via `FUN_80031558`, a DIFFERENT pool
  path `spawnEffectChild`) spawn the visual particle node.
- **Particle node handlers:** per-frame tick `FUN_80029B40`, GTE-projected renderer `FUN_80029F6C`
  (data table 0x800A1EF0+). `FUN_80029F6C` is the DRAW emitter — this is what a pc_render native
  producer must reproduce. Note: `game/render/fx_sprite.cpp` taps a DIFFERENT writer
  (`FUN_80027A4C`), so dust is not covered by it.

### THE ACTION LIST the USER asked for (which movement actions spawn dust)
Dust is requested by Tomba's motion-state dispatcher `FUN_80058918` (jump table on `actor+5`, base
0x80015CC4). Three motion states call `FUN_80053670`:
- **state 6** → `FUN_8005D16C` (WALK; calls owned `walkStart` 0x80054D14): dust on **starting to
  walk** (kind 1) and a mid-walk transition (kind 2).
- **state 5 / state 50** → `FUN_8005C8A0` (run / skid-to-turn; plays SFX 0x1D): dust on the
  **run→walk / skid** transition (kind 2, elevated p3=1).
- **state 7** → `FUN_8005CDF8` (run/turn; SFX 0x1D): dust kinds 1 and 2 with elevated spawn (p3=1) —
  this is the **"turning while running"** dust the USER named.
- Stopping / entering idle calls `FUN_80053670(...,p3=8)` = material-refresh only, NO dust.
- No dust on surface materials > 9 (deep water / special surfaces — default switch case).

So: **start-walk, walk transition, run/skid transition, turn-while-running** all kick up dust, and
the puff style depends on the ground material.

### Root cause of the pc_render gap
The ENTIRE dust family (spawn `FUN_80053670`/`FUN_800534B0` → controllers → tick `FUN_80029B40` →
render `FUN_80029F6C`) is unowned substrate. Since the break-first render rebuild (2026-07-15),
pc_render no longer walks the guest OT, so there is NO native producer for these particles → invisible
under pc_render. Identical to #12/#13/#21.

### Fix (scoped — NOT done this session)
Write a native producer tap on `FUN_80029F6C` (the dust particle renderer), exactly like
`game/render/fx_sprite.cpp` taps `FUN_80027A4C` for the torch flame: run the gen body (guest OT/pool
stay byte-exact), then re-derive the quads host-side from the particle record + GTE projection and
hand them to the render queue. Per CLAUDE.md this is deferred render work (render bugs deferred until
pc_faithful is byte-exact) and lives in game/render/ (operator-owned files), so it was scoped, not
guessed. issue8_9_re.md's texture-window analysis describes what that producer must reproduce (cell
confinement) — but its shipping-path relevance is moot now that pc_render doesn't walk the OT.

### Correcting issue8_9_re.md
issue8_9 (2026-07-10, "Walking dust clouds render as VERTICAL BARS") and this report are the SAME
effect. In 2026-07-10 pc_render walked the OT and drew the dust wrong (bars); after the 2026-07-15
OT-walk removal, pc_render lost it entirely — "drawn wrong" became "absent." Both are pc_render. The
"missing on the oracle too" half of #39 is incorrect (measured present on the oracle).

**Dead end noted:** do NOT chase "the emitter never fires." It fires. Probing `spawnEffectChild`
(`FUN_80031558`, pool path `FUN_8007A980`) is the WRONG spawn path for the dust CONTROLLER — the
controller uses `FUN_800534B0`/`FUN_8007AB20`. Also: all early null-result headless runs were invalid
because a dual-ownership `overrides::install` abort at boot (leaf_* duplicates in
field_owned_leaves.cpp, kanban #32) killed the process before any frame ran — fixed upstream in
commit d9d3787; verify boot is abort-free before trusting any spawn probe.

refs: `game/render/fx_sprite.cpp` (producer exemplar), `docs/reference/issues/issue8_9_re.md`,
scratch/screenshots/{dust_psx_310,orsweep_320,dust_pc_310}.png, scratch/decomp/dust_*.c
