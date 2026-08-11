# The unported-render inventory — every visual layer pc_render does NOT natively produce

**Opened 2026-08-06** by the G10 survey, answering the USER's request: *"Tomba! 2 also has many
unported renders so missing graphics that should be ported."*

**Why this doc and not another one.** The four tracking maps each answer one question and none of them
answers *this* one:

| map | answers | why it cannot answer "what's missing from the picture" |
|---|---|---|
| `docs/code-map.md` (`tools/codemap.py`) | WHERE a guest address is owned | a layer that was deliberately deleted looks identical to one never ported; a layer whose *entry fn* is owned can still draw nothing |
| `docs/port-map.md` (`tools/portmap.py`) | is a step REAL or a HACK | it is a step list, not a layer list — one step covers eight controllers, and a layer with no step is invisible to it |
| `docs/parity-map.md` (`tools/parity.py`) | is it SBS byte-exact | pc_render is `n/a` there by construction (it never writes guest RAM) |
| `docs/kanban/` (`tools/kanban.py`) | what a USER reported | only covers what someone happened to look at |

This file is the cross-cut: **one row per visual layer that does not reach the pc_render picture**,
with its guest producer, why it is absent, what porting it needs, and what it costs the player. It is
hand-maintained. When you close a row, close it here *and* in whichever map owns the mechanism.

**Ranking axis:** user-visible impact × how often the layer is reached × how much RE is already done.

---

## 0. THE STRUCTURAL RULE — what can reach the pc_render picture at all

This is the thing to understand before reading any row below, and it is a
definition/call-site fact, not a grep:

> `game/game_tomba2.cpp:170-176` — under `psx_render`, `Engine::drawOTag` calls
> `gpu_dma2_linked_list`. Under `pc_render` it does not, and **that is the only call site of the OT
> walk in the tree**. So *every GP0 primitive the guest links into the ordering table is
> structurally absent from the pc_render picture* unless a native producer re-emits it.

Primitives that do **not** go through the OT — the ones libgpu issues as direct GP0 words / DMA
block transfers — still execute under `pc_render`, because `GpuState::gpu_gp0_word` is reached from
the GPU register and DMA paths regardless of render mode.

### 0.1 The GP0 op-class inventory (guest emits vs native produces)

Measured on **one** run pair, so read the denominators: `PSXPORT_GATE=1`, headless, 460 frames of
`replays/bugs/bucket-softlock.pad` (area 0, the seaside), one leg with `PSXPORT_RENDER_PSX=1` and one
without, everything else identical. Counts are diagnostic lines, not pixels.

| GP0 class | decoded by | psx_render leg | pc_render leg | native counterpart | verdict |
|---|---|---|---|---|---|
| polygon `0x20–0x3F` | `gp0_exec` (gpu_native.cpp:651) | (not counted — see blind spots) | 0 via the OT | `RqItem` with `nv`=3/4, gouraud, textured, raw, semi+blend, texwindow | **producer-only**: every poly must be re-emitted natively |
| line `0x40–0x5F` | gpu_native.cpp:1029, incl. variable-length poly-lines | **1658 packets** (1064 op-0x4A, 594 op-0x5E) | **0** | no line primitive in the queue **by design** — producers expand each segment to a quad (`fx_line.cpp`) | **COVERED, and re-gated on PIXELS 2026-08-06.** All three emitters ported; census re-run over all 16 replays (97,516 packets, 0 unattributed, still exactly 3 emitters — claim C034), and the rope family proven to draw by A/B pixel diff (claim C035). Two of the four producers are still COLD — see R-CLOSED-1 |
| rect / sprite `0x60–0x7F` | gpu_native.cpp:845 | (not counted) | 0 via the OT | no rect primitive — sprite producers push quads (`push2dQuad`, `emitAnimQuadRecords`) | **producer-only** |
| FillRect `0x02` | gpu_native.cpp:958 | ≥24 (rate-limited) | **≥24, identical rects** | none needed | **NOT a gap** — it arrives on the direct-GP0 path and still executes. Every rect observed was full-screen (320×240 / 320×511), i.e. the frame clear, carrying no unique picture |
| CPU→VRAM `0xA0` | gpu_native.cpp:1186 | — | **2305 uploads** | none needed | **NOT a gap** — textures/FMV frames still land in VRAM and pc_render samples them |
| VRAM→VRAM `0x80` | gpu_native.cpp:1204 | — | — | none needed | **NOT a gap** — direct-GP0 path, still executes |
| VRAM→CPU `0xC0` | gpu_native.cpp:1102 | — | — | n/a | readback, not picture |
| env `0xE1–0xE5` | gpu_native.cpp:1130-1147 | — | — | resolved into each `RqItem` at enqueue | covered |
| mask settings `0xE6` | gpu_native.cpp:1148 | — | — | **not modelled at all** | framework-wide, affects BOTH renderers equally. Whether Tomba!2 uses the mask bit is **unchecked** |

**HEADLINE, and it is the answer to "is a whole class invisible":**
**No GP0 op class is missing from the native renderer today.** The class that *was* missing — lines,
which made every rope, chain and fishing line invisible and read as several unrelated bugs (kanban
#56) — has been closed: all three line emitters have native producers and the pc_render leg emits
zero GP0 lines while drawing them as quads.

**But the same failure shape has recurred one level down, at a SHARED WRITER instead of an op class.**
`FUN_80027768` — the effect-mesh writer, 20 controller call sites — lost its only producer route on
2026-08-04 and now draws nothing at all. That is row **R1**, and it is the single highest-value item
in this document for exactly the reason kanban #56 was: one gap, many symptoms, each of which reads
as its own unrelated missing effect.

### 0.2 Blind spots of this census — state these before citing it

1. **One replay, one area, 460 frames.** Area 0 (seaside) only. This is not the 22-area sweep
   (`docs/areas.md` recipe, instrument I021); a layer that only lives in area 7 cannot appear here.
2. **Polygon and sprite op subcodes were NOT counted, and no instrument in the tree can count them
   as-is.** `objz` is the only per-prim poly/sprite channel and it is gated twice — on
   `s_frame == s_primdump_frame` (so it prints nothing unless `PSXPORT_PRIMDUMP=<frame>` is set) and
   on `!is3d && !bg` (so it is a 2D-non-background census, never a whole-frame one). The poly/sprite
   rows above therefore rest on the call-site fact in §0, not on a count.
3. **No pictures were compared.** Nothing in this document is a pixel measurement I took; where a
   pixel number appears it is cited from the work that produced it.
4. `PSXPORT_DEBUG=nofx` is **DISTRUSTED** (instrument I018): it sits behind `fieldObjectsRender`'s
   `mem_r8(n+1) == 0` visibility skip, so it can only ever name nodes that are ALREADY VISIBLE and is
   structurally blind to a node skipped for invisibility — which is the single most important case
   for a missing-layer bug. Where I use it below, I say so. `ringcensus` (I033) is the instrument
   that sits *before* the skip.

### 0.3 UNDER `PSXPORT_GATE=1`, EVERY NATIVE OVERRIDE RUNS ITS `gen` BODY — so every diagnostic inside one is SILENT

Measured 2026-08-06, and it invalidates a whole class of "I looked and saw nothing" results.
`PSXPORT_GATE=1` sets `Game::psx_fallback = 1` (`native_boot.cpp:605`), and the ONE dispatch decision
in `override_registry.cpp:74` is

```cpp
const bool oracle = (c->game && (c->game->psx_fallback || c->game->verify.inSubstrateLeg)) || forced(e.addr);
if (oracle) { e.oracleHits++; e.gen(c); return; }
```

so **the native handler never runs.** `PSXPORT_DEBUG=ovhit` on
`replays/bugs/seesaw-weight.pad` under `PSXPORT_GATE=1` reports `native=0` for **all 482 registered
addresses** — not a sample, the whole table. Under that flag pc_render's picture comes only from the
display-pass producers reached from `Render::fieldObjectsRender` / the present path, which are plain
calls, not registry entries.

**The consequence for instruments.** A `lucent::debug` / `cfg_logf` inside a native override body
CANNOT FIRE in the standard measurement mode. Confirmed silent this way:

| channel | lives in | so under `PSXPORT_GATE=1` |
|---|---|---|
| `walk` | `Render::renderWalk` (override 0x8003C048) | prints nothing — **its silence is not a census** |
| `quadrtpt` | `QuadRtptSubmit::submitQuad` (override 0x8003B320) | prints nothing, while `ovhit` shows the address hit 14× |
| any new probe in `objListWalk1..4`, `perObjRenderDispatch`, … | overrides | prints nothing |

This is how a first attempt at the R2 reachability census read **0 across all 17 replays** while the
layer was in fact live in 4 of them: the probe had been placed in `Render::objListWalk4`. The probe
that works sits in `fieldObjectsRender` (`beamfx`), because that is not an override.

**Rule of thumb before trusting a quiet channel:** run `PSXPORT_DEBUG=ovhit` and check whether the
address owning the channel reports `native=0`. If it does, the channel measured nothing.

---

## 1. THE INVENTORY, ranked

### R1 — THE EFFECT-MESH FAMILY has no producer at all *(highest value; RE largely done)*

**FIRST CONTROLLER CLOSED 2026-08-06 — `0x8002BC9C`, the four-copy radial plume. See R1-CLOSED-1
below.** 19 of the 20 callers of the shared writer are still producer-less, so the row stays open;
what changed is that the SHAPE of the fix is now demonstrated end to end rather than described.

| | |
|---|---|
| **Looks like in game** | the weapon-IMPACT radial plume; the weapon SWING / CHARGE effect; the water jet's mesh half ("Water came out from the faucet!"); a 4-copy radial plume that is the commonest effect node in a field dump; plus up to 14 further effect controllers not yet identified by sight |
| **Guest producer** | shared writer `FUN_80027768`, reached from **20 distinct controllers**. Named ones: `0x800288AC` (impact plume), `0x8002BC9C` (4-copy plume), `0x8002A834` (SwingFx), `0x8013D454` MESH branch, `0x8013D828`, `0x8013ED08`, `0x8013EF58` (A00 overlay), and the unowned twelve `0x80028B70 0x8002C138 0x8002C6AC 0x8002CD18 0x8002D65C 0x8002DF68 0x8002F36C 0x8002FDD0 0x80030264 0x80030D68` (census in `docs/findings/render.md`, "the mesh writer has 20 callers") |
| **Why absent** | commit **`abf3cf9`** ("Delete the GTE-register render taps; four layers are now honestly absent", 2026-08-04) removed `game/render/fx_mesh.cpp/.h`, `mesh_emit_tap.cpp`, `swing_fx.cpp/.h`. `mesh_emit_tap.cpp` was the **single owner** of `FUN_80027768` and dispatched to whichever controller SCOPE was up. No scope, no picture — and pc_render does not walk the guest OT, so the guest packets are not a fallback. **The deletion was CORRECT**: those producers re-derived quads from the transform the substrate controller had just composed into GTE CR0–7, i.e. a tap, banned by PROTOCOL.md. What was never recorded anywhere is that it left the whole family producer-less |
| **Evidence** | `tools/codemap.py --addr` answers **NO NATIVE OWNER** for `0x80027768`, `0x800288AC`, `0x8002BC9C`, `0x8002A834`, `0x8013D828`, `0x8013ED08`, `0x8013EF58`. **Live confirmation on the current build, not a grep:** `PSXPORT_GATE=1` pc_render + `PSXPORT_DEBUG=nofx,ringcensus` on `bucket-softlock.pad`, 460 frames headless, names `0x8002BC9C` (node `800EE7B8`) and `0x800288AC` (node `800EEBF8`) as live-and-skipped in **area 0**, with 27 live type-0x20 nodes of 152 walked per frame |
| **Collateral** | closes-that-are-now-false: **kanban #14** ("Weapon CHARGE effect missing") was closed citing `fx_mesh.cpp` — reopened to `todo` 2026-08-06. **kanban #15**'s 2026-07-23 mesh-half fix and its 2026-07-28 A00 follow-up (gated at **700–1367 px/frame** on `walk-dust-puff.pad`) are both gone. **Claim C011** ("every type-0x20 fn the nofx census reaches now has a producer") is **falsified** |
| **Porting needs** | one native producer per controller, reading the controller's OWN node state (`node+0x48` angles, `node+0x2C/0x30` position, model table at `node+0x50`) and projecting with the native camera — the shape `fx_sprite.cpp` / `fx_dust.cpp` / `fx_line.cpp` already use. **The RE is largely DONE — do not re-derive it:** kanban #15's 2026-07-28 entry carries the full `FUN_8002BC9C` decode; `docs/findings/render.md` "The A00-overlay effect-mesh controllers" carries `0x8013D454/D828/ED08/EF58` |
| **Do NOT** | restore a scope/tap to get the picture back. `mesh_emit_tap.cpp` is not a template to resurrect |
| **Tracked as** | portmap `render-producer-effect-mesh-family` (todo, `absent:` set) + `render-producer-plume-bc9c` (ported-unverified) for the one controller now closed |

##### R1 — WORK ORDER: the 19 are gated behind 2 SHARED deps, not 19 hard ports *(measured 2026-08-11)*

`external/psxport/tools/producer_class.py` classified all 20 controllers by which GTE ops they reach.
Read the second axis, not the first — the first is what made this family look 10× harder than it is:

| axis | result |
|---|---|
| whole subtree, everything inlined | **2/20** draftable, 18 "needs judgement" — *with the identical op signature*, which is the tell |
| own ops, shared callees cut | **19/20** draftable: 14 `portable-rigid-mesh`, 5 `portable-delegates-to-shared-writer`, 1 judgement |

The lighting ops are **not in the controllers**. The controllers' own ops are `MVMVA.rot` — pure rigid
geometry. Everything else is inherited from shared callees, each ported ONCE for all of its callers:

Ownership below is from `tools/codemap.py --addr` (authoritative), **not** from reading `code-map.md`:

| shared dep | fan-in | ops | ownership |
|---|---|---|---|
| `0x80085480` | 67 | GPF×4 | **OWNED — `Math::rotmat`** (`game/math/gte_math.cpp:346`, installed :776). A rotation-matrix compose; GPF×4 is how it does the multiply. `MeshQuads::rotmat` is a second native for producers to call — not a competing claim (`codemap.py --conflicts` reports none) |
| `0x80027A4C` sprite writer | 61 | DPCS | **OWNED — `Render::fxSpriteRender`** (`game/render/fx_sprite.cpp:338`), two port-map steps `verified` |
| `0x80027768` the writer | 114 | RTPT/RTPS/AVSZ4 + **DPCT/DPCS** | **no native owner** — but its DPCT/DPCS *cue question is already answered and written down*: `game/render/fx_plume.cpp:15` records that the guest programs the cue to the IDENTITY (IR0=0, CR21-23=0), so it is a no-op in this family. Note the plume and beam ports did NOT own this function — they emit their own quads and bypass it, which is the pattern a new controller follows |
| `0x8002847C` | 26 | DPCT/DPCS | **no native owner**, and no recorded cue judgement. The one genuinely open question |

**So the gate is 2 shared deps, and only `0x8002847C` is an unanswered question** — the writer's own
cue is settled, and the plume/beam ports prove a controller can land without owning the writer. Do not
work a controller before the dep it is blocked on; that is the "jumped ahead of the frontier" failure
with extra steps.

Reproduce — and note the frontier must be GENERATED, never read off a doc:

```sh
python3 external/psxport/tools/producer_class.py --repo . selftest        # 3/3, BOTH directions
python3 external/psxport/tools/producer_class.py --repo . classify --file <addrs> --json v.json
python3 tools/producer_frontier.py --from-json v.json > scratch/producers/frontier.txt
python3 external/psxport/tools/producer_class.py --repo . classify --file <addrs> \
    --frontier scratch/producers/frontier.txt      # now blocked_on excludes what is already ported
```

**Two wrong numbers this row carried before that pipeline existed, recorded so neither is repeated.**
Without a frontier the tool reported **4** outstanding deps, because `blocked_on` lists guest functions
carrying lighting ops and says nothing about whether the port exists. Then feeding it `docs/code-map.md`
directly reported **1**, because `0x80027768` appears in that file only in the address column of a
`todo` port-map row and the scraper read "mentioned" as "owned" — the more dangerous direction, since it
retires a judgement nobody made. `producer_class.py --frontier` now refuses any line that is not a bare
address, and `tools/producer_frontier.py` resolves ownership through `codemap.py --addr`. The true count
is **2**.

Two limits on this measurement, stated because the tool's negative direction is the weak one: it is
STATIC (a controller that is never reached at runtime still classifies), and "reaches no GTE" is only
sound with zero unresolved edges — the tool reports `blind` with its denominator rather than guessing.
All 20 here walked with zero unresolved edges. Scope: this family only, not every producer in this doc.

#### R1-CLOSED-1 — the FOUR-COPY RADIAL PLUME (`FUN_8002BC9C`) *(ported 2026-08-06)*

Native producer `Render::radialPlumeRender`, `game/render/fx_plume.cpp`, portmap step
`render-producer-plume-bc9c` (**ported-unverified**). RE from ground truth `generated/shard_0.c
gen_func_8002BC9C` plus `generated/shard_5.c gen_func_80027768` for the writer.

**Why this one first:** it was the most RESIDENT unowned controller in the census (5 nodes carried it
as their `+0x18` render fn in one field dump) and it is live-and-skipped in area 0 on
`bucket-softlock.pad`, so it could be picture-verified today — unlike case-188, which cannot.

- **The controller, in its own terms.** It programs the writer's depth cue to the IDENTITY (IR0 = 0 at
  scratchpad `0x1F800090`, far colour CR21-23 = 0), so the mesh keeps its authored colours — the
  opposite of the dust puff, which publishes IR0 = `0xFFF` to have its colours REPLACED. It picks the
  mesh with the animation-script byte at `*(node+0x3C)` (low 7 bits index the node's own table at
  `*(node+0x50)`, bit 7 = the script's last frame), then draws it four times: `rotmat(node+0x48/4A/4C)`,
  column-scaled by the authored triple at `0x800A1CD4` (each byte `<< 2`), at the node's own s16 world
  position `node+0x2C/2E/30`, with sort bias `(s16)node+0x32` and no U scroll — advancing the Y angle by
  a quarter turn between copies.
- **What made it a PORT rather than the deleted tap:** the transform is built from three node angle
  fields, one node position and one authored scale triple, and projected with the native fps60-lerped
  camera. `gte_read_ctrl` appears nowhere. It is dispatched from `fieldObjectsRender`'s type-0x20 walk,
  i.e. the DISPLAY pass, so the plume is re-rendered under the lerped camera on in-between frames — the
  `plumefx` channel shows each call twice per logic frame, `t=0.50` and `t=1.00`.
- **One shared mechanism was owed and is now paid: the writer's own ORDERING decision.** `FUN_80027768`
  averages the four projected depths (AVSZ4), adds the CALLER's sort bias, compresses to an
  ordering-table bucket and drops anything outside `[4, 2048)`. `MeshOtBias` (`mesh_quads.h`) carries
  that, and the OT-unit→view-unit factor is derived from the game's OWN authored constant — its
  projection init `gen_func_80083FF8` sets `ZSF4 = 256`, so the key is mean-depth/4 and one bias unit is
  four view units. **No GTE control register is read to get it.** It is opt-in: the two pre-existing
  callers of the shared walk (`fx_dust`, `narration_swirl`) have not had their bias arguments RE'd, and
  claiming them would be jumping ahead of the RE, so they are byte-for-byte unchanged.
- **Proven to draw, with a producer-disabled negative control, in the producer's own active window.**
  Two Release binaries built in the ISOLATED tree `psx/scratch-plumeab/T2` (distinct md5s), identical
  except the one dispatch branch; same replay, headless, `PSXPORT_GATE=1` pc_render,
  `PSXPORT_PRESENT_SHOT_AT` at 960×720. Active window (`plumefx`: f252–f263) — present **254: 675
  changed px** of 691,200, bbox x[465,515] y[273,299]; **258: 3267 px**, bbox x[432,545] y[261,368];
  **262: 828 px**, bbox x[432,551] y[291,341]. Outside it — presents **300 and 320: 0 px**, where the
  producer is never called. Leg proof is in-band and not the pixels: **24 `plumefx` lines ON, 0 OFF**.
  The diff mask is ONE connected radial cluster around Tomba's head, and the ON crop shows white/yellow
  spikes that the OFF crop does not have.
- **AND THE CHANGED PIXELS ARE WHERE THE PRODUCER SAID THEY WOULD BE.** `plumefx` now reports the screen
  box the call actually emitted into, so the diff is checkable against the producer's own prediction
  rather than merely being non-zero somewhere: scaled by ires 3, **675/675 (100%)** of the f254 diff,
  **828/828 (100%)** of the f262 diff and **3258/3267 (99.72%)** of the f258 diff fall inside it.
- **The 9 pixels that do not, recorded rather than smoothed over.** They are ONE native 320×240 pixel
  (146,122) — a 3×3 block at (438..440, 366..368) in the 960×720 shot — which goes from pale yellow
  (206,206,107) with the producer OFF to dark brown (107,74,49) with it ON. It is well outside the
  plume's own footprint, so the plume did not paint it; the shape of the change (one pixel of one
  surface losing to another) points at a depth-coincidence/ordering flip of the kind kanban #74 tracks,
  triggered by the extra draws. **Not root-caused.** Anyone touching this layer should start here.
- **What is NOT verified:** no USER eyeball. The controller's SECOND half — when `node+3` is 0x14/0x15
  it hands the list at `node+0x34` to the SPRITE writer `FUN_80027A4C` — is NOT ported; every observed
  call carries subtype 0x07, so that branch is *unreached*, not broken, and `plumefx` tags it
  `[sprite-half reached — NOT PORTED]` the moment a scene takes it. And no cross-check against
  `psx_render` was made: the two legs' present-frame timelines are offset (the psx leg skips the OP
  FMV), so the same frame number is a different moment, and aligning them was not attempted.

### R2 — The `submitQuad` caller classes: flames, generic particles, ~~see-saw/beam quads~~

**BEAM QUADS CLOSED 2026-08-06** — see R2-CLOSED-1 below. Two classes remain open.

| | |
|---|---|
| **Looks like in game** | the A00-overlay flame/rope emitter's quads; generic particle quads (case-188) |
| **Guest producer** | `FUN_8003B320` (`submitQuad`, already native) called from **`renderWalkCase188`** (generic particles) and the a00-overlay emitter around `0x801341xx` |
| **Why absent** | DELETED 2026-08-04 with `quad_rtpt_submit.cpp`'s GTE-register tap. Their display-pass records were built from `gte_read_ctrl(0..4)/(5+i)` after the substrate's RTPT ran, then un-composed against the scene camera — the exact banned shape |
| **Porting needs** | port each emitter and draw from its own world state; there is no shared shortcut, which is precisely why the shared tap existed |
| **OPEN before porting** | the **CR contract**, *per emitter*. B704's answer (it loads the pure camera itself) does **not** transfer: `renderWalkCase188` loads CR0–7 from `CASE188_SCR`, the a00 emitter from whatever it composed. Read each body's own `SetRotMatrix`/`SetTransMatrix` argument — that is where the answer lives, and it is a two-minute read, not a research project |
| **REACHABILITY — case-188 is a blocker, not a porting task** | its dispatch target `0x8003C188` is **never taken in any of the 17 replays**, so a port could not be picture-verified today (the same trap as `fx-jet-mesh-sprite-10c1d8`). **And the instrument that would census it is blind:** `PSXPORT_DEBUG=walk` lives inside `Render::renderWalk`, which is an *override* — see §0.3. Its silence is not evidence. A capture that reaches case-188 is the prerequisite |
| **Tracked as** | portmap `render-producer-submitquad-classes` (todo, `absent:` set); `docs/fps60-rework.md` residuals |

#### R2-CLOSED-1 — the BEAM / see-saw ribbon (`FUN_8003B704`) *(ported 2026-08-06)*

Native producer `Render::beamQuadRender`, `game/render/fx_beam.cpp`, portmap step
`render-producer-beam-b704` (**ported-unverified**). RE from ground truth
`generated/shard_0.c gen_func_8003B704`.

**The CR-contract question this row carried as OPEN is answered, and the answer was inside the
emitter all along:** it calls `func_80084660` / `func_80084690` (libgte `SetRotMatrix` /
`SetTransMatrix`) with `a0 = 0x1F8000F8` — **the pure camera** — overwriting whatever
`perObjRenderDispatch` / `billboardCompose1` left in CR0–7, immediately before it builds its corners.
So the corners are **world space**, and the producer needs nothing but the node's own state plus the
native camera. No register read-back, no tap.

- **Geometry:** half-extent `H = 0x14 · (cos a·cos b, sin b, −sin a·cos b)` from `node+0x68` (azimuth)
  and `node+0x6A` (polar, `+1024` when `*(u8*)0x800E7FC6 < 4`); the span runs from the tracked anchor
  `*(0x800E7F5C)` (s32 at `+0x2C/30/34`) to the node's own position (s16 at `+0x2E/32/36`), split at
  the round-toward-zero midpoint when `(s16)node+0x60 == 3`. Each span is the quad `(P−H, Q−H, P+H, Q+H)`.
- **Two corrections to the older decode in `docs/fps60-rework.md`, both from ground truth:**
  `DAT_800a3b04[node+0x66*2]` is **not a colour** — it is the pair of texture **V rows** (the packet's
  `v0/v1` and `v2/v3`); the U columns are fixed at 224/247. And the packet's RGB word is *never
  written at all*, which is consistent because GP0 code `0x2D` has the RAW bit set, so the texel is
  unmodulated. (Also: the *table* is dumped as `80 87 88 8F 90 9F 80 87` — index 3 aliases index 0,
  which is why two different `uvIdx` values print the same V pair; that is the data, not a bug.)
- **Proven to draw, with a negative control.** Two binaries identical except this producer (the
  NO-BEAM leg built in an isolated tree, both `Release`), same replay
  `replays/bugs/weapon-impact-bucket.pad`, headless: f652 **84 changed pixels** in bbox
  `x[153,179] y[120,125]`, inside the producer's own reported screen bbox
  `[145.9,117.3]..[188.8,129.3]`; f646 26 px, f648 7 px, f650 **0 px** — and f650's zero is the
  honest one: the producer's own log says the span was degenerate (`A == N`), i.e. a zero-area quad.
  **No pixel outside the beam's bbox changed in any of the four frames.**
- **What is NOT verified:** only the single-span form (`kind != 3`) and only the `0x8003EF30` arm were
  ever reached. The split form and the billboard arm (`0x8003EF40`, gated `node+2 == 1`) are
  unexercised across the whole replay library, and no USER has looked at the layer.
- **Reachability census** (`PSXPORT_DEBUG=beamfx`, 900 frames each, all 17 replays, and the summary
  line carries its denominator): `weapon-impact-bucket` 52 producer calls, `save-sign-softlock` 42,
  `seesaw-weight` 28, `walk-dust-puff` 28, the other 13 replays 0.

### R3 — `FUN_8013CDD4`'s GT4 prop quads (drum / windmill caps)

Deliberately absent since 2026-08-04 (same tap deletion). **Confirmed live in area 0 this session**:
`nofx` names node `800EE598` render fn `0x8013CDD4`; the library census recorded it in 15 of 17
replays, so this is a commonly-reached layer, not a corner case. `WidescreenMarginQuad::emit` still
*exists and installs* — `codemap.py --addr 0x8013CDD4` finds an owner — which is exactly why the
codemap alone cannot answer "is this layer present". **Real fix:** drive the emitter from the node's
own position + rotation angles (`obj+44` / `node+0..2`, the inputs `0x800318A0` composes), never by
reading back what it composed. Tracked as portmap `render-producer-margin-quad`.

### R4 — Overlay-mode geomblks: the mesh-flush SEAM

`Render::subPartWalk` / the shared per-cmd flush own the **GENERIC** mode loop only. Overlay-mode
geomblks (`0x8012xxxx` / `0x8013xxxx`) are the next tier and are explicitly marked **do NOT jump**.
This is a whole-tier gap rather than one effect, and it sits under several of the rows below.
Tracked as portmap `render-mesh-flush` (blocked).

### R5 — Area backdrops and ambient layers, per area

| area | layer | guest producer | state |
|---|---|---|---|
| 21 | sky is a **GRADIENT + tilemap composite** — the gouraud base is unported | `gen_func_8003DF04` special-case → `0x8010BE30` → helper `ov_a0l_gen_8010BB64` (four POLY_G quads spanning x[0,320], colours `0x00AC0606` / `0x00EA9898`, scroll-derived Y from the s16 at `0x800C00F0`) | kanban #49 todo. Routing it through the plain tilemap producer was **tried and measured worse** — do not retry that |
| 14 | waterfall backdrop's **sprite tail** — ~~unported~~ | `FUN_80110CA4` tail-calls `0x801104D0`, 440 gen lines | **NOT A GAP — kanban #67 is STALE.** `codemap --addr 0x801104D0` returns `Render::fxBackdropSparkRender` LIVE (`fx_backdrop_plane.cpp:210`), called from `fxBackdropPlaneRender` exactly as the guest tail-calls it. Its card's stated blocker (34 `FUN_8009A450` calls writing the seed) does not apply: the guest's own body keeps the pool simulated underneath, so the producer only READS slot state — the randomness is upstream. Status is **ported-unverified**, because the pool reads `live=0/200` in the only capture: an EMPTY POOL, not a dead producer. Needs a scene that populates it, not a port |
| 21 | the **jet effect** | `FUN_8010C1D8` (A0L) | kanban #66 todo, blocked: returns immediately unless `*(u8*)0x800BFA55 >= 4`, and it reads 1 in the standard capture. Port is otherwise ready |
| 4 | ambient effect + a **342-point tile field** | `FUN_8013B118`; the field is `ov_a04_func_8013AD90` (218 lines of raw GP0 tile emit, **no analogue anywhere in `game/render/`**) | kanban #68 todo. Of its three stated blockers, **the PRNG one is resolved**: `GuestRngMirror` (`game/render/guest_rng_mirror.{h,cpp}`) exists, is in `cmake/tomba2_port.cmake`, and is a per-logic-frame read-only seed snapshot built for exactly this. The two that remain are real: every branch is gated off in the only reachable state (story phase `0x800E7EAA` = 1), and the 342-point field has no analogue in the tree. The field deserves its own row once reachable |

### R6 — Layers with a producer that still never draw *(NOT missing producers — different fix)*

Ranked here because the user sees them as missing graphics, but the fix is not "port an emitter":

- **Stunned-enemy spinning stars** (kanban #55 / #72). The producer EXISTS and is a real native one
  (`Render::fxSpriteEmit`'s `FN_RINGROT` branch, `0x8002B3A4`, zero `gte_read_*`). The ring node
  spawns, but `node+1 == 0` (never marked visible) and it self-retires on its first behaviour tick
  because `gen_func_8002B7B0` tests `mem_r8(mem_r32(node+0x14)+0x1B) & 0x40` and `node+0x14` is 0 —
  **nothing in the spawn path writes it**. NEXT RE STEP: who is supposed to write `node+0x14` for
  behaviour `0x8002B7B0`. *This session's ringcensus on area 0 reported `rings=0` on every frame —
  no stun occurs in that replay, so it neither confirms nor denies the card.*
- **Bucket's supporting POLE** visible only after pickup (kanban #54) — state-gated, not class-gated.
- **Hut/house interior wall decorations** (kanban #57, #74) — suspected occlusion / coincident-face
  ordering, i.e. a depth problem in a layer that IS produced.
- **GAME OVER screen keeps drawing field geometry** (kanban #62) and **DEMO transition submits ~125k
  garbage quads from a geomblk pointing at texture data** (kanban #70) — wrong-content, not absent.
- **Flying bird in area 0** (kanban #63) — missing, mechanism not yet attributed to a class.

### R7 — 2D layer residuals

`field-2D layer (#3b)` is ported-unverified as a whole. Named remaining gap: **special-character icon
glyphs `FUN_80078988` are still substrate**. Also outstanding on that step: the gauge firing drive and
a USER eyeball of the whole 2D layer.

### R8 — Implemented but never seen on screen ("ported-unverified" — plausible and unproven)

Each of these is code that exists and may be silently wrong; treat a green gate on them as hollow
until something drives them. `fxAltAnimSpriteRender` (`0x8012E868`) and `fxRotSpriteTailRender`
(`0x8012D9E8`, **and its inline rotated-mesh pass is unported**) are COLD across the entire 15-replay
library — zero emissions, checked by their distinguishing gate/depth signature rather than assumed.
`fadeTileRender` (`0x800726D4`) — cold because the game has two unrelated fade paths and this is the
rare one. `composeTintGate` (`0x8003EF9C`) and `sharedTransformWalk` (`0x8003F07C`) — cold, need a
scene using render mode 2. `impactRingRender` / `impactAnnulusDraw` (`0x8002ECD8` / `0x8002E680`) —
no replay in the library reaches them. **Dust PUFF MESH layer** (kanban #53) — implemented from the
RE, ring state never leaves {0,1} in 4000 frames so it has never been exercised.
`world-line-ring-shadow` — see R-CLOSED-1. **Added 2026-08-06 by the pixel-gate pass:**
`Render::ropeChainRender` (`0x8013EA64`, the 8-point chain) is COLD across the entire 16-replay
library — it never appears as an emitter on the psx leg either, so nothing has ever driven it; and
`Render::shockwaveRingRender` (`0x8013E08C`) is called 152× in `bucket-softlock` and contributes
**zero pixels** (claim C036), consistent with being off-screen there. Both are "implemented and never
seen", which is exactly this row's category — they are NOT covered by R-CLOSED-1's rope evidence.

### R9 — Whole SCENES with no producer (abort, not a missing layer)

`Render::renderScene` dispatches six scene kinds — `Loading`, `StartBoot`, `Title`, `Field`,
`HutInterior`, `SopNarration` — and **aborts** on anything else (`render_walk.cpp:350`), as does a
DEMO front-end substate outside `sm[0x48] ∈ {2,3,4,6,7}` (`render_walk.cpp:403`). That abort is the
honest design (no OT-walk fallback), but it means an unported scene class is a crash, not a blank
layer. Reaching a new scene kind is therefore a porting *prerequisite*, and `PSXPORT_RENDER_PSX=1` is
the way to drive into one.

---

## 2. DEPENDENCY ORDER — what to work, and in what order

```
  R1  effect-mesh family (FUN_80027768 x 20 controllers)      <- START HERE
       |   independent of everything below; RE already done; one producer per controller,
       |   and each one landed is a whole visible effect back
       |
  R4  overlay-mode geomblk mesh flush (the SEAM)
       |   several R5/R8 layers sit on this tier; portmap says do NOT jump ahead of it
       |
       +--> R2 submitQuad classes (B704 / case188 / a00 flame)
       |       gated on answering the CR-contract question first, from GAME state
       +--> R3 FUN_8013CDD4 prop quads       (independent; smallest of the three tap victims)
       |
  R5  per-area backdrops
       |   area 21 gradient  -- independent, ready. THE ONLY ONE THAT NEEDS PORTING WORK.
       |   area 14 tail      -- NOT a gap; ported. Needs a scene that fills the pool.
       |   area 21 jet / area 4 -- blocked on REACHABILITY, not on code. Needs a scene where
       |                     the phase byte advances; that is a game-driving task, not RE.
       |
  R6  behaviour-link bugs (stars: who writes node+0x14) -- independent of all render work
  R7  2D residual: FUN_80078988 special-char glyphs      -- independent
  R8  drive the cold producers                            -- needs scenarios, not code
```

**Two chokepoints worth naming**, because each unblocks several rows at once:

1. **scene reachability, and it is the bigger one.** Area 21's jet, area 4's ambient, the dust puff
   mesh (#53) and every cold producer in R8 are blocked on *reaching a game state*, not on RE or
   code. That is a driving / replay-capture task, it is cheap relative to porting, and it would
   convert a large block of "ported-unverified" into either verified or a real bug. **Doing this
   before more porting is probably the highest return per hour on this whole list**, because an
   unverified rebuild is exactly the class kanban #10 exists to warn about.
2. **the host-side guest-PRNG mirror is NOT a chokepoint any more** — recorded here because two
   cards still say it is. `FUN_8009A450` does read *and write* the seed at `0x80105EE8` (claim
   **C018**, kanban **#69**), but `GuestRngMirror` (`game/render/guest_rng_mirror.{h,cpp}`, in the
   CMake source list, already referenced from `fx_backdrop_plane.cpp`) is the per-logic-frame
   read-only snapshot that answers it. Do not re-derive that design.

---

## 3. HOW TO RE-RUN THIS CENSUS

```sh
# the GP0 class census — TWO LEGS, and the psx leg is the negative control that proves the
# instrument is not silent. Everything else identical between them.
printf 'newgame\nrun 460\nquit\n' | PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1 PSXPORT_REPL=1 \
  PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_WATCHDOG=300 \
  PSXPORT_DEBUG=fillrect,lineprim,upload \
  PSXPORT_PAD_REPLAY=replays/bugs/bucket-softlock.pad ./scratch/bin/tomba2_port 2> scratch/g10/psx.err
# ...then the same command WITHOUT PSXPORT_RENDER_PSX=1 -> scratch/g10/pc.err
grep -o 'op=0x[0-9A-F]*' scratch/g10/psx.err | sort | uniq -c     # line packets by op

# which type-0x20 nodes the native walk has no producer for (I018 — DISTRUSTED denominator:
# blind to INVISIBLE nodes) plus the ring census that sits BEFORE the visibility skip (I033)
PSXPORT_DEBUG=nofx,ringcensus   # same run shape, pc_render leg

# who owns a guest address, with the denominator of what was searched
python3 tools/codemap.py --addr 0x800288AC
```

**Two traps this census hit, recorded so nobody re-hits them:**

- `PSXPORT_DEBUG=objz` **prints nothing** unless `PSXPORT_PRIMDUMP=<frame>` is also set — the log
  line is gated on `s_frame == s_primdump_frame` — and even then it is a 2D-non-background census
  (`!is3d && !bg`), never a whole-frame op histogram. A quiet `objz` is not a negative result.
- The `fillrect` channel is **rate-limited** to the first 24 lines plus every 512th, and the counter
  it prints is the point of the line. Seeing `#24` as the last line means the true total is
  somewhere in [24, 512) — do not read 24 as the count.

---

## 4. DOC / TRACKER DEFECTS FOUND BY THIS SURVEY (all corrected in the same pass)

1. **Claim C011 falsified** — "every type-0x20 render fn the nofx census reaches now has a producer"
   rested on three of its five fns being "owned by another route"; `abf3cf9` deleted that route.
2. **portmap `world-line-ring-shadow` was STALE** and was inflating the missing-layer list — it
   listed `FUN_8013E08C` as todo/BLOCKED on RE of `FUN_80084110/80084220`, but the *same address* is
   step `fx-line-emitter-e08c`, ported 2026-07-28 as `Render::shockwaveRingRender`, whitelisted, and
   `codemap --addr` returns it LIVE. Corrected to ported-unverified. **Unresolved and now recorded:**
   this step calls the layer a *ground ring shadow* and `fx_line.cpp` calls it an *expanding
   shockwave ring* — same code, two incompatible descriptions, nobody has looked at it.
3. **kanban #14 reopened** — closed citing a file that no longer exists.
4. **kanban #67 is STALE** — it says the area-14 backdrop's sprite tail `0x801104D0` "is unported"
   and blocked on the guest PRNG. `codemap --addr` returns `Render::fxBackdropSparkRender` LIVE, and
   the port's own note explains why the PRNG constraint does not apply to it. Noted on the card.
5. **kanban #68's PRNG blocker is stale** — `GuestRngMirror` exists and is built. Its other two
   blockers (state gating, the 342-point field) are real. Noted on the card.
6. **Stale comments in `game/render/render_walk.cpp` (lines ~794, ~802, ~850) and
   `game/render/fx_sprite.cpp` (~251, ~343) and `render.h` (~281)** still say the mesh half is
   "already captured by fx_mesh's armTap scope" / "owned by fx_mesh.cpp's a00 scope". `fx_mesh.cpp`
   was deleted. **These comments are the reason R1 stayed invisible for two days** — they read as an
   ownership record. They are NOT corrected by this doc (the survey was told not to change renderer
   code); correcting them is a one-line-each follow-up and should happen with R1.

*Pattern worth naming: four of the six defects above are a tracker row that outlived its own fix.
Sizing this work from the card titles alone would have OVER-counted the missing layers by two (#67,
`world-line-ring-shadow`) and UNDER-counted them by one — R1, which no tracker mentioned at all and
which is the largest item on the list. That asymmetry is the argument for this file existing.*

### R-CLOSED-1 — the LINE class *(kept here because it is the template for R1)*

Kanban **#56** ("SYSTEMIC: pc_render has NO line-primitive producer — every GP0 line is invisible")
is the canonical instance of the failure this document exists to catch: *one structural gap that
presents as several unrelated missing effects* (bucket rope, fishing line, hanging vines). It is now
**closed pending a USER eyeball**. All three emitters have native producers — `FUN_8013E9D8`
(anchor rope), `FUN_8013EA64` (8-point chain), `FUN_80122974` (4-mode tether incl. the fishing line),
all through the shared leaf `FUN_8013DD34`, plus `FUN_8013E08C` (the ring). Measured this session on
the same replay: psx leg **1658** GP0 line packets, pc leg **0** line packets and instead **152**
shockwave + **598** rope-leaf producer calls. The queue stayed quads-only — the segment→quad
expansion lives in the producer, which is the pattern R1's port should follow.

*Un-root-caused, recorded rather than hidden:* 152 shockwave producer calls is exactly 2× the 76 that
the guest's 1064 op-0x4A packets imply (7 spans × 2 strokes per call). Most likely the fps60
real+interp double render — but that is an inference, not a measurement.

**2026-08-06, SECOND PASS — the close above rested on a COUNT; it now rests on PIXELS, and the
denominator went from one replay to sixteen.** Two things were wrong with the row as written:

1. *"pc leg emits 0 line packets and instead fires the native producers 152 + 598 times"* is a
   primitive count. A count cannot distinguish a producer that DREW from one that emitted into a
   batch nobody rendered — which is the mistake claim **C131** is on file for. Redone as an A/B pixel
   gate (claim **C035**, instrument **I038**): four separately-built binaries, distinct md5s, from an
   ISOLATED tree — never the shared checkout — same replay/frames, headless, `PSXPORT_GATE=1`
   pc_render, `PSXPORT_PRESENT_SHOT_AT` → `gpu_vk_present_shot` at 960×720.
   **Rope producers live vs deleted: 1584 / 1197 / 927 changed px of 691,200 at presents 440/445/450**,
   bbox 41×263 / 41×206 / 29×164, tracking right as the camera pans; the diff mask is one continuous
   curved line (`scratch/lineclass/evidence/present_44*_OFF_ON_DIFF.png`). Negative control: 0 px on
   presents 455–485, where the producer is not called at all (last call f453). Sensitivity: a 12×-wider
   white stroke gives 19107 px = 12.06×, bbox +66 px = 22 px × ires 3.
2. *"ONE replay, ONE area"* was also the whole basis for "no fourth emitter". The `lineprim` census is
   now run over the **entire 16-replay library** (claim **C034**): **97,516 line packets, 0
   unattributed, exactly THREE emitter addresses**, all three with a LIVE native producer. That is the
   falsifier C031 named and nobody had run.

**AND AN INSTRUMENT WAS CAUGHT LYING doing it — record before citing any preseq-based picture result.**
The first attempt at this gate used the REPL `preseq` dump. `preseq` goes through
`gpu_vk.cpp dump_to()` → `readback_vram()`, a **320×240 VRAM readback**, not the presented picture — so
it is structurally blind to a DISPLAY-PASS producer like this one (`render_walk.cpp` defers these to
the present-time re-render). A leg with the rope stroke widened to **24 px and forced to pure white**,
598 draws logged, diffed **0 changed pixels** against a leg with the producer deleted. Filed as
**I037, DISTRUSTED**. An instrument that cannot see a 24 px white line is not evidence that a 2 px grey
one is absent.

**What this row does NOT cover — two of the four line producers are still COLD:**

| producer | addr | state after this pass |
|---|---|---|
| `worldLineDraw` via `ropeAnchorRender` / `tetherLineRender` | `0x8013E9D8` / `0x80122974` | **VERIFIED ON PIXELS** (C035) |
| `ropeChainRender` | `0x8013EA64` | **COLD — never appears as an emitter in ANY of the 16 replays.** The chain is implemented and has never been exercised in *either* renderer. Belongs in **R8**, not in a closed row |
| `shockwaveRingRender` | `0x8013E08C` | ~~COLD — 152 calls, ZERO pixels~~ **WRONG READING, CORRECTED 2026-08-06 — it was BROKEN, and it is now FIXED and pixel-verified** (C036 falsified → C037). See below |

**CORRECTION, 2026-08-06 — the "COLD" cell above was false and is kept struck through so the mistake
is legible.** The zero was real; the inference was not. The A/B behind it captured presents **440-485**
while the producer fires only at **f270..f358** — the window held zero of its calls, and "the node is at
world (7,200,10), so it is off-screen" was a *symptom of the port bug* read as an innocent explanation.
The same census log this row cites shows the guest drawing that ring at screen (160,163) — dead centre —
for ~90 frames.

**ROOT CAUSE (two independent bugs, either one fatal on its own).** `Render::shockwaveRingRender` took
its object translation from `node+0x4E` (the ROPE/TETHER node family's layout) while the emitter
`FUN_8013E08C` submits the packed SVECTOR at `node+0x2C` to `0x80084220`; on a ring node `0x50` is the
*scale animator*, so the port's Y was the ring's own radius and X/Z came from unrelated fields. And
`Robj` was divided by 4096 although `projComposeObjectHost` takes 1.3.12 (4096 = identity), collapsing
every ring to a single point.

**PIXEL-VERIFIED, in the producer's OWN active window** (`scratch/ring/ring_gate.sh` in an isolated
tree, three separately-built binaries with distinct md5s and in-band leg proof, `PSXPORT_GATE=1`
pc_render, `PSXPORT_PRESENT_SHOT_AT` → 960×720 present shots, presents 275/280/287/320/340/355):
fixed-vs-deleted = **450 / 657 / 909 / 2151 / 1602 / 2232 changed px of 691,200**, and every diff mask
is a single closed ellipse outline with no stray region anywhere on screen. **NEGATIVE CONTROL:** the
*shipped* producer against the same deleted leg, same presents, same differ = **0 on all six** — so the
instrument reproduces the reported failure before it reports the fix.

**WHICH VISUAL IT IS, settled:** an **expanding shockwave ring**, not a ground ring shadow. The
`lineprim` log shows it growing from ~6 px across at f270 to ~80 px at f357 while its grey fades
122 → 13, and the native producer's projected screen box now tracks the guest's own submitted packet
vertices to ~1 px on 8 sampled frames. Rename the port-map step `world-line-ring-shadow` accordingly.

**The 152-vs-76 factor of 2 is root-caused and is NOT a bug:** the 152 calls are exactly 76 distinct
(frame, node) pairs seen twice — one call per PRESENT, because fps60 re-renders the field object walk
for the interpolated present. Measured, not inferred: with `fps60=0` in `psxport_settings.ini` the same
replay logs **76** calls, matching the guest's 1064 packets ÷ 14 per call exactly.
