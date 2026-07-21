
## Object position is 16.16 FIXED POINT — the u32 at +0x2C/0x30/0x34 aliases posX/posY/posZ
- **status:** RE'd + named in `Actor` (2026-07-21).
- **why it matters:** motion handlers looked like they used two unrelated position fields — a 32-bit
  one they integrate into (`obj+0x30`) and a 16-bit one everything else reads (`obj+0x32`). They are
  the SAME field. The u32 is 16.16 fixed point and, little-endian, its HIGH halfword *is* the 16-bit
  integer position; the low halfword is the sub-pixel fraction.
  ```
  +0x2C u32 = X fixed   -> high half is posX (+0x2E)
  +0x30 u32 = Y fixed   -> high half is posY (+0x32)
  +0x34 u32 = Z fixed   -> high half is posZ (+0x36)
  ```
- **velocity / acceleration:** `+0x48 velX · +0x4A velY · +0x4C velZ` (i16, 1/256 world units per
  frame) and `+0x50 accelY`. The per-frame step is a textbook integration:
  ```
  posFixed += vel * 0x100;      // 1/256 units -> 16.16
  vel      += accel;
  ```
- **evidence:** `release_trigger_motion.cpp` performs exactly that pair on all three axes
  (0x2C/0x48, 0x30/0x4A/0x50, 0x34/0x4C); the arc motions seed velY with 0xE800 / 0xF000 — negative
  i16, i.e. an upward launch, matching hover/swoop/arc behaviour. Independently,
  `beh_variant_actor_sm.cpp` writes `0x26DE0000` to `+0x2C`: integer X in the high half, zero
  fraction in the low half — which only makes sense under this layout.
- **consequence for readers:** a 32-bit write at +0x30 CHANGES posY. Anything reasoning about
  position must treat the two as one field, and any lens must expose both views (Actor now does:
  `posYFixed()` / `posY()`).

### Ranking files by OFFSET alone is meaningless — the base struct decides
After naming the 16.16 position / velocity fields in `Actor`, I ranked "files still using raw
0x2c/0x30/0x34/0x48/0x4a/0x4c/0x50" to find more conversion targets. That ranking is worthless on its
own, because those offsets mean different things in different records:

| file | base variable | what the record actually is |
|---|---|---|
| `scene/demo.cpp` | `sm` | the DEMO state machine — `sm[0x48]` is the SUBSTATE INDEX, not velX |
| `core/engine.cpp` | `sm`, `task` | state-machine / task records |
| `object/actor_sm_reward.cpp` | `e`, `entA`, `entB`, `eA`, `eB` (30 sites) | entity records, NOT the `obj` node |
| `ai/release_trigger_motion.cpp` | `anchor` (16 sites) | a different object than the `obj` converted there |

Converting `demo.cpp`'s `sm + 0x48` with `Actor::velX()` would have been badly wrong — the same
offset, a completely different struct, and the code would still compile and mostly still run.

**Before converting any raw offset to a lens accessor, establish what the BASE VARIABLE points at.**
The offset is not the identity; the record is. This is the same hazard as `0xC0` meaning `childPtr`
in the scene-graph role and `renderRec` in the behaviour role — see `Actor`'s and `Node`'s headers,
which both document it.
