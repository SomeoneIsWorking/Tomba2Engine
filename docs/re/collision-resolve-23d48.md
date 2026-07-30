# FUN_80023D48 — actor-vs-object CYLINDER COLLISION RESOLVE (RE, not yet ported)

RE'd 2026-07-29 via Ghidra headless (`scratch/decomp/eng_23d48.c`) + `abi_extract.py --contract`
against `generated/shard_1.c gen_func_80023D48`. **PORTED 2026-07-29** (`game/world/collision_resolve.cpp`,
SBS 0-diff over 1500 frames, ovhit 7397/7397, port_check PASS) — via `port_gen.py`, which emits the
`func_XXXX` calls and so handles the stack hazard below BY CONSTRUCTION rather than by care.

**READABILITY PASS COMPLETE 2026-07-30.** Both bodies (`cylinderResolve` 0x80023D48 and
`landOnObjectTop` 0x8002423C) now read as named control flow over named values: the `L_8002xxxx`
gotos are gone, the register chains are named locals or `GuestReg<N>` proxies, the frame is a
`GuestFrame<80,10>` / `GuestFrame<32,3>` built from the `abi_extract --scaffold --guestabi` contract,
and the eight call sites are `guest_call(c, kRa…, func_XXXX)`. Re-gated: port_check PASS on both
methods, and the recorded 1500-frame SBS gate re-run gives 50/50 A/B-identical checkpoints with
ovhit 0x80023D48 native=7397 oracle=7397 and 0x8002423C native=1949 oracle=1949.

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

## How it decides (established 2026-07-30 during the readability pass)

Two gates, then a shallowest-axis choice. `anchor` supplies the POSITION being resolved against and
`other` supplies the DIMENSIONS — they are separate arguments because the caller resolves against a
hitbox whose size and whose placement live in different records.

1. **XZ gate.** `dist = sqrtLzc(dx² + dz²)` from the sample point to the anchor. Reject (v0 = 0) when
   `(actor+0x82 - actor+0x80) + other+0x80 < (dist & 0xFFFF)`.
2. **Vertical gate.** `restOffsetOnTop = other+0x84 + (actor+0x86 - actor+0x84)` — where the actor's
   origin sits when it is resting on the top face. Reject (v0 = 0) when
   `(s16)actor+0x86 + (s16)other+0x86 < ((Δy + restOffsetOnTop) & 0xFFFF)`, where
   `Δy = actor+0x32 + yBias - anchor+0x30`.
3. **Sign split on Δy.** Guest Y grows DOWNWARD. `Δy < 0` (actor ABOVE the anchor) negates both the
   gap and the snap, so the "rest on top" snap is NEGATIVE; `Δy >= 0` (actor below) recomputes the
   snap as `actor+0x84 + (other+0x86 - other+0x84)`, the push-out-underneath offset. The vertical
   REACH stays positive on both sides.
4. **Shallowest axis wins.** `xzPenetration = radiusSum - (s16)dist` vs
   `yPenetration = (s16)reach - (s16)gap`. `xzPenetration < yPenetration` → horizontal push-out
   (v0 = 1); otherwise the vertical snap, which is v0 = 3 when the snap is `> 0` (pushed underneath)
   and v0 = 2 + the landed flag when it is `<= 0` (resting on top). So outcome 2 being the NEGATIVE
   snap is not an oddity — it is the downward-Y convention.

## Two radii out of one pair — an OBSERVATION, not a renaming

The actor's `+0x80`/`+0x82` are named `kActorHeightLo`/`kActorHeightHi` in the port, but this
function uses them as XZ radii: the overlap GATE uses `(+0x82 - +0x80) + other+0x80` while the
push-out places the actor on a contact circle of radius `+0x80 + other+0x80`. FUN_8002423C
independently uses `actor+0x80 + other+0x80` as its XZ reach. Two different radii come out of the
same pair, and no source (USER or guest data) has been found for what the pair actually is, so the
names are LEFT ALONE — an offset is a fact, a field name is a claim. Anyone who finds the real
meaning should rename in both tables at once.

## Callee audit — what the register file can and cannot leak (2026-07-30)

Needed before any register chain could be folded into a C++ local, because a callee that spills its
caller's callee-saved registers, or stashes its incoming a0..a3 into the O32 argument-save area at
the top of OUR frame, would put a stale register value into guest RAM that SBS compares. Scanned
transitively over the five leaves and the one real nested call:

* `0x80083F50` rcos, `0x80084080` sqrtLzc, `0x80085690` ratan2, `0x80077768` angleCmp — frame_size 0,
  **no memory writes at all**. (Each gen body has a trailing `func_XXXX(c)` after its `return;`; that
  is the recompiler's dead-sibling fall-through, not a call.)
* `0x80083E80` rsin — frame_size 24, writes exactly one word: its own `ra` at its `sp+16`, i.e. BELOW
  our sp. Its real callee `0x80083EBC` writes nothing.

So nothing this function calls can observe a t-register, and the single guest-stack word any of them
writes is precisely the one the `func_XXXX` wrappers exist to preserve (see THE HAZARD below). The
port therefore keeps r16..r23/r30 and the per-call-site argument registers on the register file and
folds only `r2/r3/r8/r9` plus dead `r4/r5` intermediates into named locals.

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
