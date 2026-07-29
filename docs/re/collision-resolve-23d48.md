# FUN_80023D48 — actor-vs-object CYLINDER COLLISION RESOLVE (RE, not yet ported)

RE'd 2026-07-29 via Ghidra headless (`scratch/decomp/eng_23d48.c`) + `abi_extract.py --contract`
against `generated/shard_1.c gen_func_80023D48`. **NOT PORTED, and deliberately no draft was
written** — see the stack hazard below, which is what actually makes this a hard port rather than a
long one. Recorded so the next session starts from the analysis instead of the disassembly.

Why it matters: **29,869 substrate dispatches per 6000 frames** of `replays/bugs/seesaw-weight.pad`
— the busiest remaining unowned function in the game once the PlatformHle-owned entries are
discounted (see instrument I024).

## Signature

    FUN_80023D48(actor = a0, other = a1, anchor = a2, flags = a3) -> v0

`v0` is a 4-valued outcome, not a boolean:

| v0 | meaning |
|----|---------|
| 0  | no collision — the separation test failed, nothing written |
| 1  | HORIZONTAL push-out applied (actor X/Z moved to the contact circle) |
| 2  | vertical resolve AND the "landed" flag `actor+0x29 = 1` set |
| 3  | vertical resolve only |

`flags & 1` selects between two entry geometries: clear = use the actor's raw position; set = offset
the sample point by `radius(+0x7c)` rotated by `angle(+0x56)` first, and carry that offset back out
of the final write. The two branches converge on shared separation/response code.

## Field map (as used here — offsets are role-specific, per docs/findings/object.md)

* actor: `+0x2E` X, `+0x32` Y, `+0x36` Z (X/Z as u16, Y also written as 16.16 at `+0x30`),
  `+0x56` angle, `+0x7C` sample radius, `+0x7E` Y bias, `+0x80`/`+0x82` height band,
  `+0x84`/`+0x86` vertical extents, `+0x0C` type (case 2 updates facing), `+0x5F` facing result,
  `+0x60` facing reference, `+0x29` landed flag.
* other: `+0x80` radius, `+0x84`/`+0x86` vertical extents.
* anchor: `+0x2C` X, `+0x30` Y (also read as a 32-bit 16.16), `+0x34` Z.
* Writes scratchpad `0x1F80009C` with the `ratan2` contact angle.

## THE HAZARD — this is a stack-fidelity port, not just a logic port

Eight call sites, all `direct target=func_XXXX`, to five leaves that are ALL already owned natively:
`0x80084080` Math::sqrtLzc, `0x80083F50` Trig::rcos, `0x80083E80` Trig::rsin, `0x80085690`
Trig::ratan2, `0x80077768` Trig::angleCmp.

That looks like the port is nearly free. It is not. **`Trig::registerOverrides` is deliberately an
empty body** (see the banner in `game/math/trig.cpp`): those substrate bodies descend guest stack
frames the native methods do not mirror, so registering them diverges SBS. Calling the Trig METHODS
directly from a native — which is the sanctioned use elsewhere, e.g. `game/render/fx_trail.cpp` —
would leave the callees' frame bytes BELOW this function's sp unwritten, while substrate core B
writes them. SBS compares that memory. That is the dead-stack-scratch divergence class, and
CLAUDE.md's rule is to mirror the frame, never to exclude the region.

**So a port of this function must invoke the leaves through their generated `func_XXXX` wrappers**
(the `guest_fn` helper in `runtime/recomp/guest_abi.h` is the existing vocabulary for this — see
`game/render/node_xform.cpp`), so each callee's own frame is written exactly as the substrate writes
it. Porting the logic and calling `trigOf(c).rcos(...)` instead would look correct, build clean, and
diverge.

## Own frame contract (from abi_extract, do not hand-derive)

`frame_size = 80`. Ten callee-saved spills in program order: r17@+44, r22@+64, r30@+72, r31@+76,
r23@+68, r21@+60, r20@+56, r19@+52, r18@+48, r16@+40. Three s16 locals at sp+16, sp+24, sp+32 — the
`sStack_40/38/30` of the decompile — written on BOTH entry branches but with different values, and
read back near the exits. `abi_extract` flags those three reads as "no matching prologue spill",
which is correct and expected: they are locals, not register saves.
