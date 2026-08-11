# `FUN_800288AC` — the weapon IMPACT radial plume (mesh half)

**Status: RE COMPLETE, NOT PORTED.** Decoded 2026-08-11 from ground truth `generated/shard_5.c`
`gen_func_800288AC` (lines 2410-2574, 164 raw lines, 9 of 13 blocks reachable — the other 4 are the
recompiler's folded-in trailing function) plus `gen_func_80027768` (the shared 36-byte-record mesh
writer). **It is NOT ported because the decode found a parameter the shared native writer does not
model — see §4, which is the whole reason this file exists rather than a producer.**

Prior art that must not be re-derived: this controller WAS ported once, as `FxMesh` in
`game/render/fx_mesh.cpp` (kanban #15, 2026-07-23), and that file was **deleted in `abf3cf9`** because
it derived its transform from `gte_read_ctrl(0..7)` — a tap, banned by `PROTOCOL.md`. The deletion was
correct. What follows resolves the same transform from NODE STATE instead, which is what makes a
display-pass producer legitimate (and interpolatable at fps60).

Reached as a type-0x20 node render fn (`node+0x18 == 0x800288AC`), i.e. through
`Render::fieldObjectsRender`'s whitelist — not by any static caller. The impact effect has TWO halves:
`FUN_80033080 = { FUN_80027E5C(); FUN_800288AC(node); }`. The sprite half already renders through
`fxSpriteRender`; this mesh half is the two `0x3E` gouraud quads that ARE the radial plume.

Live reachability, measured not assumed: `PSXPORT_GATE=1` pc_render + `PSXPORT_DEBUG=nofx,ringcensus`
on `replays/bugs/bucket-softlock.pad`, 460 frames headless, names `0x800288AC` (node `800EEBF8`) as
live-and-skipped in area 0. The original fix's repro is `replays/bugs/weapon-impact-bucket.pad`
**f654-660**, evidence bbox f656 `x[115,190) y[80,175)`.

---

## 1. The algorithm

```
FUN_800288AC(node):                      # a0 = node = r18;  0x1F800000 = scratchpad
  rec = *(u32*)(node + 0x3C)             # the ANIMATION SCRIPT cursor (4-byte records)
  if (rec == 0) return                   # no script -> emit nothing. The honest early-out.

  # --- object transform, entirely from the node's OWN state ---
  rotmat(&node[0x48], 0x1F800000)                 # FUN_80085480 = Math::rotmat, three Euler angles
                                                  #   node+0x48 / +0x4A / +0x4C
  colScale = { rec[0] << 2, rec[1] << 2, rec[2] << 2 }      # the SCRIPT's own scale triple
  matColScale(0x1F800000, colScale)               # FUN_80084520 = Math::matColScale

  # compose object rotation with the SCENE CAMERA rotation (camera CR0-4 from 0x1F8000F8),
  # three column MVMVAs (0x4A49E012 = mx=rot, v=IR, cv=none) writing back into 0x1F800000
  # translation: MVMVA (0x4A486012 = mx=rot, v=V0) on *(u32*)(node+0x2C) / *(u32*)(node+0x30),
  #              then ADD the camera translation at 0x1F80010C/110/114 -> 0x1F800014
  # publish composed rotation to CR0-4 and translation to CR5-7

  # --- the DEPTH CUE, driven from the script attribute byte ---
  attr = (u8)rec[3]
  if (attr & 0x40) {
      *(u32*)0x1F800090 = 4096 - ((attr & 0x3F) << 6)       # IR0: a REAL driven cue
      a1 = 0
  } else {
      *(u32*)0x1F800090 = 0                                  # IR0 = 0, cue is the identity
      a1 = attr & 0x0F                                       # see section 4
  }

  # --- advance the script, honouring the last-frame bit ---
  if (attr & 0x80) *(u32*)(node + 0x40) = 0                  # script ended
  else             *(u32*)(node + 0x40) = rec + 4            # next 4-byte record

  CR21 = CR22 = CR23 = 0                                     # far colour black: cue fades toward black
  FUN_80027768(a0 = *(u32*)(node + 0x50),                    # the mesh record list
               a1 = a1,                                      # section 4 — THE UNMODELLED ONE
               a2 = (s16)*(node + 0x32),                      # sort bias
               a3 = (u8)*(node + 0x29))                        # U offset
```

**It emits ONCE.** Contrast `FUN_8002BC9C` (the four-copy plume, ported as `Render::radialPlumeRender`),
which calls the writer four times advancing the Y angle a quarter turn between copies.

## 2. Node and script fields

| field | type | meaning |
|---|---|---|
| `node+0x29` | u8 | writer `a3` — U offset added to the four packet U bytes |
| `node+0x2C/0x30` | u32,u32 | world position, as the GTE `VXY0`/`VZ0` pair |
| `node+0x32` | s16 | the writer's sort bias (`MeshOtBias`) |
| `node+0x3C` | u32 | animation-script cursor, 4-byte records. **0 = emit nothing** |
| `node+0x40` | u32 | where the advanced cursor is stored back |
| `node+0x48/4A/4C` | s16 ×3 | the Euler angles `Math::rotmat` consumes |
| `node+0x50` | u32 | the mesh record list handed to the writer |
| `rec[0..2]` | u8 ×3 | column-scale triple, each `<< 2` |
| `rec[3]` | u8 | `attr`: bit7 = last frame, bit6 = drive the cue, bits5-0 = cue amount, bits3-0 = §4 |

## 3. What the writer does with each argument (`gen_func_80027768`)

| arg | treatment |
|---|---|
| `a1` (r5) | `r5 << 22`, then ADDED to the record's word0 before it is stored into the packet |
| `a2` (r6) | `r6 << 16` — the sort-bias half, already owned by `MeshOtBias` (`mesh_quads.h`) |
| `a3` (r7) | added to FOUR packet U bytes by read-modify-write **on the PACKET, not the record** — so the record is left clean and a read-only native producer MUST apply this offset itself. It maps exactly onto `meshQuadRecordsEmit`'s `uBias`, which adds to the record's U at draw time |

## 4. WHY THIS IS NOT PORTED — `a1` is a CLUT-row bias the native writer cannot express

`Render::meshQuadRecordsEmit` (`game/render/mesh_quads.cpp:131`) takes the CLUT straight from the
record: `const uint16_t clut = (uint16_t)(w0 >> 16);` (line 182). The guest instead adds `a1 << 22` to
that same word0 before building the packet. Bit 22 is **bit 6 of the `w0 >> 16` CLUT field**, and in a
PSX texpage/CLUT word the CLUT's low 6 bits are its X and the bits above are its Y — so `a1` shifts the
**CLUT ROW**, i.e. it selects a different palette. `a1 = attr & 0x0F` comes from the script, so the
effect is authored to change palette per script frame, and it is only non-zero on the branch where the
depth cue is the identity — the two mechanisms are alternatives, which is consistent with both being
ways of recolouring the same quads.

A producer that ignores `a1` draws the plume in the wrong palette on every record whose
`attr & 0x0F != 0`, and — this is the part that makes ignoring it unacceptable rather than approximate —
it would do so SILENTLY, since nothing compares native CLUT against guest CLUT.

**The fix is small and additive, and it is a PORT, not a judgement call:** give
`meshQuadRecordsEmit` an opt-in CLUT-row bias parameter, defaulted so the existing callers are
bit-identical. That is deliberately the same discipline `MeshOtBias` already follows — opt-in with
`known=false`, so `fx_dust` / `narration_swirl` / `fx_plume` are untouched and no caller's un-RE'd
argument is silently claimed. Do NOT widen it into a general "packet word0 bias": what is RE'd here is
a CLUT row, and naming it that keeps the next reader honest.

Sequence, so nothing jumps the frontier:
1. extend `meshQuadRecordsEmit` with the opt-in CLUT-row bias; prove the existing three callers are
   unchanged (identical pixels on a replay that reaches each);
2. then write `Render::impactPlumeRender` per §1, dispatched from the type-0x20 whitelist on
   `node+0x18 == 0x800288AC`;
3. gate it the way `render-producer-plume-bc9c` was gated — two binaries from an ISOLATED clone
   differing only in the one dispatch branch, `replays/bugs/weapon-impact-bucket.pad` f654-660, an
   in-band leg proof (a diagnostic line count, never the pixels being measured), and the producer's own
   reported bbox checked against the diff mask.

## 5. Traps already paid for once — carried forward from kanban #15

- **`drawWorldQuad` sets `has_xyf`**, which makes fps60 skip a guest-execution-time prim at present: the
  effect is submitted every frame and yet zero pixels change. A display-pass producer is the shape that
  avoids this; do not re-diagnose it as a missing emission.
- **The burst needs `node+0x32`'s sort bias applied to depth or the bucket occludes it.** `MeshOtBias`
  exists for exactly this and takes the bias explicitly.
- **The attack button is CIRCLE, not SQUARE.** Every "not reproduced" verdict on kanban #15 before
  2026-07-23 held square and was therefore measuring nothing.
