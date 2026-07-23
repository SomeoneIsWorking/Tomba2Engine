# Effects — spawn/render findings

## [effects] Movement DUST CLOUDS — RE'd and PORTED (kanban #39)

**Symptom (USER 2026-07-23):** Tomba should kick up dust clouds on movement actions (starting to
walk, turning while running, others). Reported missing under pc_render AND "under the oracle."

**Status:** FIXED 2026-07-23 — native producer `Render::dustEffectRender` (game/render/fx_dust.cpp),
dispatched from `fieldObjectsRender`'s type-0x20 walk. The card's premise ("missing on the oracle too")
was FALSIFIED earlier: the effect spawns and renders fine on psx_render/ORACLE and was missing ONLY
under pc_render — the same missing-native-producer class as #12/#13/#21.

### THE ACTION LIST the USER asked for (which movement actions spawn dust)
Requested by Tomba's motion-state dispatcher `FUN_80058918` (jump table on `actor+5`, base 0x80015CC4).
Three motion states call the dust-spawn decision `FUN_80053670`:
- **state 6** -> `FUN_8005D16C` (WALK; calls owned `walkStart` 0x80054D14): dust on **starting to walk**
  (kind 1) and on a mid-walk transition (kind 2).
- **state 5 / state 50** -> `FUN_8005C8A0` (run / skid-to-turn, SFX 0x1D): dust on the **run->walk /
  skid** transition (kind 2, elevated p3=1).
- **state 7** -> `FUN_8005CDF8` (run/turn, SFX 0x1D): kinds 1 and 2 with elevated spawn — the
  **turning while running** dust the USER named.
Stopping / entering idle calls `FUN_80053670(..., p3=8)` = surface-material refresh only, NO spawn.
No dust on surface materials > 9. Puff STYLE is per-surface: subtype = `FUN_800535D4(actor)` =
`actor[0x176] + 1`, and every table the renderer reads is indexed by it.

### The chain, end to end
`FUN_80053670` (decision) -> `FUN_800534B0` (pool spawn, `FUN_8007AB20`) -> the dust CONTROLLER
(handler table @0x800A4480 by subtype-1: `FUN_8006AE28` / `FUN_8006BB6C` / `FUN_8006C478` /
`FUN_80069300`) -> its state-entry helper calls **`FUN_80031558`**, which allocates the VISUAL node and
stamps it: `node+0x0B = 0x20` (custom-render-fn node), `node+0x1C = FUN_80029B40` (the position-history
ring behaviour, already owned as `beh_pos_history_trail`), `node+0x18 = FUN_80029F6C` (the renderer),
`node+0x10 = the controller`, `node+3 = subtype`. That last stamp is the decisive fact: the dust's
dispatch point is `fieldObjectsRender`'s **type-0x20** branch on render fn `0x80029F6C`, exactly like
the torch flame's.

### What FUN_80029F6C draws (the RE the producer implements)
Two layers, both from the node's own state:
1. **TRAIL** (`FUN_80029664`, unconditional): the first FOUR ring slots (node+0x38, stride 8) projected
   through the pure scene camera; each consecutive visible pair becomes TWO untextured gouraud quads
   (GP0 0x3A), one per side, outer edge black and inner edge the per-subtype colour ramp at
   `0x800A1EF0[sub]` (5 words). Half-width = `0x800A1FA4[sub]` byte per point, turned into a SCREEN-space
   perpendicular (`ratan2(dy,dx) + 0x400`, then `rcos` -> X and `rsin` -> Y — note that order, it is easy
   to swap) and scaled per endpoint by that endpoint's depth-cue factor (DQA=6 => MAC0 = 6n). Blend =
   guest tpage 53 => abr 1 = ADDITIVE.
2. **PUFF** (`FUN_80027768`, gated on `node+5` in {2,3} and `(node+0x36 & 7) < 3`): the packed mesh at
   0x8009F3E0 drawn FOUR times, 0x400 apart. Transform = `rotmat(0, owner+0x68, node+0x36 - 0x400)` then
   `rotY(i*0x400)`, column-scaled by `0x800A1FEC[node+6]` bytes<<2, translated to node+0x30/32/34.
   Subtypes >= 6 take the sibling path: base matrix = the owner's matrix block (owner+0x98), second
   rotation = `rotZ`, scale = the fixed triple at 0x800A1CD4, mesh = 0x8009F398. The emitter publishes
   depth-cue IR0 = 0xFFF and far colour `0x800A1FC8[sub]`<<4, so the mesh colours are REPLACED by the
   per-surface tint. U scroll = low byte of `node+0x36 << 5` (the texture frame walk).

### The port
`game/render/fx_dust.cpp`. Real port, not a tap: no `gen_func_*` runs for the picture and no `gte_op` is
used. The transform chain is rebuilt host-side by the shared `MeshQuads` helpers (game/render/
mesh_quads.{h,cpp}, which also now own the ONE walk of the 36-byte packed-mesh record format that
narration_swirl.cpp used to keep privately), composed with the fps60-lerped camera via
`projComposeObjectHost`, and submitted as float world quads.

**Evidence (seesaw-weight.pad, the dust node 0x800EEE18 lives f216-231):** at frame 226 the guest
submits six 0x3A quads with first vertices (190,66) (194,74) (198,77) (206,60) (210,68) (210,75)
(`debug otattr` under PSXPORT_RENDER_PSX=1); the native producer's six quads at the same frame are
(191,66) (195,75) (198,77) (207,60) (211,69) (210,77) — every vertex within 2 px, the residual being the
float-vs-fixed projection. `debug fxdust` prints the per-quad vertices and the frame/t stamp.

**fps60:** the effect's world points go through the new `EffectLerp` tier (game/render/effect_lerp.
{h,cpp}) — captured per logic frame keyed by node, blended at `Fps60::mT` on the in-between present, so
the SAME producer draws both frames. Measured on the same replay (seg 1, side 0, vertex 0 x): real f224
= 214, interp f225 = 210, real f225 = 207; real f225 = 207, interp f226 = 193, real f226 = 182; real
f226 = 182, interp f227 = 173, real f227 = 163. Each in-between sits between its two real neighbours.
Ring lerping is index-wise, which is correct because slot i advances by exactly one sample per frame.

### Not exercised: the PUFF mesh layer
Across 4000 frames of seesaw-weight the dust node's `node+5` never leaves {0,1}, so the guest itself
never emits the mesh layer there (`otattr` shows only 0x3A trail quads) and neither does the producer.
The mesh path is implemented from the RE but has NOT been seen on screen — treat it as ported-unverified
until a repro reaches ring state 2/3.

### Sibling family fixed in the same pass
Hunting the dust surfaced a SECOND missing producer: emitter `FUN_800286CC` + packet writer
`FUN_8002847C` — 36-byte four-corner quad records selected by an animation-script byte (node+0x3C ->
index into the record-list table at node+0x50), per-axis scale pair at node+0x34, world anchor and OT
bias at node+0x2C/0x30/0x32 exactly like the FUN_80027A4C flame family. That is the impact starburst
(and other one-shot puffs). Now `Render::fxAnimSpriteRender` in game/render/fx_sprite.cpp. Evidence at
seesaw-weight f755: strongly-magenta pixel count psx_render 2703, pc_render 877 before / 2956 after,
centroid (197.7,144.0) vs (189.8,136.0). Its node+0x32 near bias is honoured as real view-Z (4x the
bucket bias) — without it the burst loses its near half to whatever it bursts on.

### Dead ends (do NOT re-chase)
- "The emitter never fires." It fires.
- `FUN_80029B40` is NOT a dust-specific behaviour — it is the generic position-history ring, and
  `codemap --addr 80029B40` has pointed at `beh_pos_history_trail` the whole time.
- The 4-sprite ring emitter `FUN_8002B3A4` (also a FUN_80027A4C-family node) is NOT the dust; it is a
  static ambient effect anchored to a fixed world point.
- A pc_render run cannot be attributed with `otattr`'s packet-span list (only the GTE histogram is
  populated) — compare against a separate PSXPORT_RENDER_PSX=1 run, and align on the frame number the
  `[otattr] frame=N` header prints, which is NOT the REPL's last `run` count.

refs: `game/render/fx_dust.cpp`, `game/render/mesh_quads.{h,cpp}`, `game/render/effect_lerp.{h,cpp}`,
`game/render/fx_sprite.cpp`, `game/render/render_walk.cpp` (type-0x20 dispatch),
`game/ai/beh_pos_history_trail.cpp`, `docs/reference/issues/issue8_9_re.md`

