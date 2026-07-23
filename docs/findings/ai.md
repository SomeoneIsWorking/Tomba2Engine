# Findings — AI / enemy behavior

## ActorMeleeEngage::doIt — scratchpad 0x1F80009C stale-write on the "arm-directly" path (RESOLVED, 2026-07-10)

- **symptom:** two independent frontier-convergence agents (2026-07-10) reproduced an SBS-full
  divergence around f1019 at scratchpad `0x1F80009C`, surfacing "inside `Trig::ratan2`
  (0x80085690, already-owned) with mismatched return addresses between core A/B" — one via
  `PSXPORT_DEBUG=ovhit PSXPORT_REPL=1 PSXPORT_SBS_AUTONAV=1` during a melee encounter, one via
  git-stash A/B on unmodified `main`. Both agents correctly flagged it as pre-existing and out of
  their pass's scope (see `docs/findings/render.md`'s "PutDrawEnv cluster" entry, "unrelated
  timing-sensitive finding"). The standard 95s `SBS_MODE=full SBS_AUTONAV=1` gate (no `ovhit`)
  does NOT hit it — it only manifests once autonav timing happens to walk into a melee fight,
  which the extra `ovhit` bookkeeping overhead was observed to perturb into reaching earlier.
- **root cause (line-by-line RE cross-check, `generated/ov_a00_shard_1.c:3527`,
  `ov_a00_gen_80112188`):** ground truth's approach-angle scratch store,
  `mem_w32((r20 + 156), r4)` (i.e. `mem_w32(0x1F80009C, angle)`), sits in the **delay slot** of
  the `bne` that branches on `margin < bandWidth` (gen line ~3603-3606):
  ```c
  { int _t = (c->r[2] != c->r[0]); c->mem_w32((c->r[20] + (uint32_t)156), c->r[4]); if (_t) goto L_80112320; }
  ```
  Per MIPS delay-slot semantics the store executes UNCONDITIONALLY every time control reaches this
  branch — on BOTH the "reposition" path (`margin < bandWidth`, jumps to `L_80112320`) AND the
  "margin >= bandWidth" fallthrough that goes on to test `margin < 3` (armed-directly tail,
  `L_801124C0`, when that's also false). The native port (`game/ai/actor_melee_engage.cpp`,
  `ActorMeleeEngage::doIt`) had gated this write inside `if (margin < bandWidth)`, so on the
  "arm-directly" branch (`margin >= bandWidth && margin >= 3`) the write was silently dropped,
  leaving scratchpad `0x1F80009C` stale on core A (pc_faithful) while core B (gen/oracle) always
  refreshed it on every call that got past the XZ-distance and Y-band early-outs. Content-
  dependent: only bites when a melee AI actor's `doIt()` call takes that specific branch, matching
  the "only reproduces mid-fight, not on generic autonav" symptom. The "mismatched RA inside
  Trig::ratan2" framing from the original repro was a downstream artifact of the SBS write-watch
  attributing the (missing) write to whichever call frame happened to touch that scratch word next
  — not a bug in `Trig::ratan2` itself (independently line-traced against `gen_func_80085690`,
  `generated/shard_4.c:13206`, and found byte-faithful for the branches walked: x/y sign-strip,
  the `a0&0x7FE00000`/`a1&0x7FE00000` overflow-guard split, and the `a0/(a1>>10)` vs
  `(a0<<10)/a1` division selection all match).
- **fix:** moved the `c->mem_w32(0x1F80009Cu, (uint32_t)angle)` store to execute unconditionally
  right after computing `angle`/`margin`/`bandWidth` (matching the delay-slot's unconditional
  semantics), collapsing the old if/else into `const bool doReposition = (margin < bandWidth) ||
  (margin < 3);`. `game/ai/actor_melee_engage.cpp`.
- **sibling checked, no bug found:** `MeleeProximity::isAtApproachAnchor`
  (`game/ai/melee_proximity.cpp`, `gen_func_8001F9DC`, `generated/shard_2.c:795`) writes the same
  scratch word, but ground truth's store there (`generated/shard_2.c:848`) is a plain sequential
  instruction on the success path only (not a delay-slot write) — the native port already matches.
- **verification:**
  - Line-by-line RE cross-check of `gen_func_80112188` against the native port (this session) —
    confirmed the delay-slot bug above; no other discrepancies found on the paths traced (kindZBias
    polarity, dz/dx computation, Y-band test, reachHi/reachLo split, ratan2 argument order/rsin-
    rcos ordering all independently re-verified and matched the existing "BUG FIX" comments from
    the prior wide-RE pass).
  - Standard gate: `timeout 95 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_SBS_NOPAUSE=1` —
    0-diff through f8550+, SIGINT-terminated by the watchdog, no crash (no regression from the fix).
  - Repro invocation: `PSXPORT_DEBUG=ovhit PSXPORT_REPL=1 PSXPORT_SBS_MODE=full
    PSXPORT_SBS_AUTONAV=1 PSXPORT_SBS_NOPAUSE=1` (150s) and the same + `PSXPORT_SBS_POSTDRIVE=1`
    (300s) — 0-diff through f11100+ / f12540+ respectively; the divergence did not reproduce in
    either window. **Caveat, stated honestly:** neither run's autonav actually reached a melee
    encounter — a standalone `PSXPORT_MIRROR_VERIFY=0x80112188,0x8001F9DC` pass (15000 frames,
    `PSXPORT_SBS_EXIT_FRAME=15000` for a clean atexit dump) confirms `0x80112188`
    (`ActorMeleeEngage::doIt`) and `0x8001F9DC` (`MeleeProximity::isAtApproachAnchor`) both show
    `NEVER HIT` under the current autonav+postdrive script on this route — the fixed-script
    "hold Right + periodic Cross-tap" autonav does not currently walk into an enemy on this level
    within the windows tested. The fix is verified by the RE cross-check + no-regression gates, NOT
    by re-observing the original divergence disappear live — a future session with autonav that
    actually reaches a melee fight (or a scripted pad-replay into one) should re-confirm 0-diff
    live. This is a genuine gap in the fleet's autonav tooling (it never exercises enemy combat),
    worth flagging for a future workflow improvement, not just this bug.
- **refs:** `game/ai/actor_melee_engage.cpp`, `game/ai/melee_proximity.cpp`,
  `generated/ov_a00_shard_1.c:3527`, `generated/shard_2.c:795`, `generated/shard_4.c:13206`,
  `docs/findings/render.md` ("PutDrawEnv cluster" entry, original repro note).
# Findings — AI / combat

## Combat-cluster autonav coverage gap — CLOSED for 2/3 addresses; combat-leg SBS divergence RESOLVED (2026-07-10)

- **symptom (the gap):** `ActorMeleeEngage::doIt` (0x80112188), `MeleeProximity::isAtApproachAnchor`
  (0x8001F9DC), and `beh_actor_tomba_proximity_combat` (0x800527C8) were all wired/registered but
  their "verified" status rested ENTIRELY on the RE line-by-line cross-check against
  `generated/`, never on a real SBS run — the standard gate's autonav (`PSXPORT_SBS_AUTONAV=1`)
  never leaves the immediate seaside spawn area, so `PSXPORT_DEBUG=ovhit` read `A=0 B(gen)=0` for
  all three under the standard 95s gate command, confirmed live:
  `timeout 100 env PSXPORT_VK_HEADLESS=1 PSXPORT_SBS=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1
  PSXPORT_NOAUDIO=1 PSXPORT_SBS_NOPAUSE=1 PSXPORT_DEBUG=ovhit ./scratch/bin/tomba2_port` — all
  three read `NEVER HIT` at 600+ frames.
- **fix — `PSXPORT_SBS_AUTONAV=combat`** (runtime/recomp/sbs.cpp, `sbsCombatOn()` + the `Nav::DONE`
  combat leg): a NEW opt-in knob, off by default so the standard gate is unaffected. Deterministic
  input script (required — SBS runs two cores in lockstep off identical input, no wall-clock/RNG
  navigation allowed): once player control is reached, hold Right continuously and fire a Cross
  jump-edge every 60 frames starting at frame 300 (post-control). This walks Tomba past the seaside
  spawn's `ActorZonedAttacker` encounter (`id_compare_motion_dispatch`, node handler 0x80145230,
  physical collision wall at world Z≈6190) and deep into the same corridor's `cull_substate_
  orchestrator` cluster (handler 0x8013259C, object 0x800F1190 by the time coverage fires).
- **REAL BUG FOUND WHILE BUILDING THIS: `BTN_RIGHT` was `0x2000` (Circle) not `0x0020` (Right)**
  in `runtime/recomp/sbs.cpp`'s pad-button constants (see the `BTN_RIGHT FIX` comment at the
  constant's definition). This is a pre-existing bug, NOT introduced by this task — it also broke
  `PSXPORT_SBS_POSTDRIVE=1`'s existing "walk into things" script (Nav::DONE), which has been
  pressing Circle instead of walking Right since that knob was introduced (2026-07-08). Fixed by
  correcting the constant to `0x0020` per the pad table in `docs/driving-the-game.md`. Verified
  the fix doesn't perturb the standard gate: `PSXPORT_SBS_AUTONAV=1` (no combat leg), 95s window,
  0-diff through f10470 (`scratch/sbs_standard_gate.log`, not committed).
- **coverage result (`PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=combat PSXPORT_SBS_EXIT_FRAME=2500
  PSXPORT_DEBUG=ovhit`, headless, ~40s):**
  - `0x80112188 ActorMeleeEngage::doIt` — **A=690 hits** (fires every frame once Tomba is near the
    `cull_substate_orchestrator` objects, even before fully closing the distance — its own early-out
    XZ/Y-band gates return 0 for most calls, but the call itself is real per-frame coverage).
  - `0x8001F9DC MeleeProximity::isAtApproachAnchor` — **A=1662 hits** (starts firing once
    `ActorMeleeEngage::doIt`'s `doReposition` path arms the object — confirms the two-address
    cascade RE'd in `game/ai/actor_melee_engage.h`/`melee_proximity.h`).
  - `0x800527C8 beh_actor_tomba_proximity_combat` — **still 0 hits** even at frame 2500 / world
    Z=14003 (well past both prior obstacles). Its call site is an INDIRECT per-object "think"
    pointer (no static `func_800527C8(c)` call site exists anywhere in `generated/`) — no spawned
    object in the ~150-entity reachable seaside/intro area (confirmed via a full live `ents` walk,
    `tools/dbgclient.py`) has its think-slot pointed at this address. Reaching it needs either a
    wider-area playthrough (past whatever area transition spawns the object that uses it) or
    directly RE'ing which spawn table entry installs this handler. **STILL OPEN** — correctness for
    this address rests on the RE verification in `beh_actor_tomba_proximity_combat.h`, not gate
    coverage, same caveat as `docs/fleet-workflow.md` §9's standing rule.
  - `MeleeProximity::isAtApproachAnchor` and `ActorMeleeEngage::doIt` themselves show `A=N B(gen)=0`
    ("COUNT MISMATCH vs substrate") in the `ovhit` dump — this is the SAME known tooling caveat as
    `quadrtpt`/`ovgtgnd`/other direct-`g_override`-wired A00-overlay leaves (`docs/config.md`
    "ovhit" section, item 2): `noteSubstrateDispatch` isn't wired for every direct/g_override call
    site, so `B(gen)`'s count is a metric-tracking gap, not evidence the override didn't fire on
    core B — `sbs-div` (the real byte-level RAM/scratchpad compare) is the trustworthy signal.
- **SBS divergence discovered by this coverage — RESOLVED (2026-07-10, same-day follow-up session).**
  5 identical `[sbs-div]` hits, all at the same byte, f807-f811:
  `[sbs-div] f807 0x1F80009D..0x1F80009E (1 B)  A=00  B=08` (and f808/809/810/811, same values).
  `0x1F80009C` is the "shared approach-angle scratch word" `ActorMeleeEngage::doIt` stamps
  (`game/ai/actor_melee_engage.cpp`) — core A (native) left the byte at 0x1F80009D as stale 0x00,
  core B (oracle/substrate) wrote fresh 0x08.
  - **Original triage (this entry, first pass) mischaracterized this as an UPSTREAM
    register/stack-faithfulness divergence** ("stack-depth OPEN" cluster) because a targeted
    `MIRROR_VERIFY=0x80112188,0x8001F9DC` pass showed zero mismatches in `doIt`'s own body,
    implying the bad state must already exist on entry. That reasoning was correct as far as it
    went but missed the actual mechanism: **the shape `A=00 (stale) / B=08 (fresh)` is the exact
    signature of a dropped WRITE, not a wrong VALUE computed from bad inputs.**
  - **Actual root cause: the SAME bug as this file's first entry above**
    ("`ActorMeleeEngage::doIt` — scratchpad 0x1F80009C stale-write on the arm-directly path",
    fixed in commit 76227b8, landed in this branch's history AFTER the combat-leg coverage that
    surfaced this paragraph was originally written, but BEFORE this follow-up session started).
    `gen_func_80112188`'s `mem_w32(scratch+156, angle)` sits in the delay slot of the
    `margin < bandWidth` branch and fires unconditionally; the pre-fix native port gated it behind
    `if (margin < bandWidth)`, dropping the store on the "arm-directly" branch
    (`margin >= bandWidth && margin >= 3`) and leaving 0x1F80009C/9D stale on core A whenever a
    melee actor's `doIt()` call took that branch — exactly reproducing `A=00 (never refreshed)
    B=08 (oracle always refreshes)`. There was never a second, distinct upstream bug here; the
    combat leg's coverage-gap discovery and the RE-derived fix happened to be authored in
    different sessions before the fix's effect on THIS specific repro was re-verified live.
  - **Re-verified live (2026-07-10 follow-up, worktree `agent-acd7fa85ffaccbf1a`, tip
    `4ebaec6` rebased):** the exact repro command below now runs 0-diff through f9510 (95s
    watchdog window, no early exit), well past the old f807-811 hit — the divergence no longer
    reproduces. Standard gate (`PSXPORT_SBS_AUTONAV=1`) independently re-confirmed 0-diff through
    f10440 in the same session, so the fix does not regress the default leg either.
  - **repro (now clean):** `timeout 100 env PSXPORT_VK_HEADLESS=1 PSXPORT_SBS=1
    PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=combat PSXPORT_NOAUDIO=1 PSXPORT_SBS_NOPAUSE=1
    ./scratch/bin/tomba2_port` — 0-diff to the watchdog limit (was `[sbs-div]` at f807-811,
    `0x1F80009D`, `A=00 B=08` before commit 76227b8).
  - **process lesson:** when a fix targets scratch word X via an RE-identified mechanism (dropped
    delay-slot store), re-check EVERY other repro that touches the same word before filing it as a
    separate/upstream issue — `A=stale / B=fresh` on the same address the fix touches is a strong
    prior for "already fixed, not yet re-verified" over "new bug in a different layer".
- **gate policy:** `PSXPORT_SBS_AUTONAV=combat` stays OFF by default (still an OPTIONAL deeper gate
  for sessions specifically working the combat/AI cluster, not wired into the standard command) —
  but as of the 2026-07-10 follow-up above it is ALSO verified 0-diff through f9510 (95s window),
  same as the standard `=1` leg (0-diff through f10440+). Nothing currently blocks promoting it to
  the default gate; it remains opt-in only because `docs/fleet-workflow.md` §2's standard command
  hasn't been updated to include it, not because of any known-red issue.
- **refs:** `runtime/recomp/sbs.cpp` (`sbsCombatOn()`, `Nav::DONE`'s combat leg, `BTN_RIGHT` fix),
  `game/ai/actor_melee_engage.{h,cpp}`, `game/ai/melee_proximity.{h,cpp}`,
  `game/ai/beh_actor_tomba_proximity_combat.{h,cpp}`, `docs/config.md` ("ovhit" +
  "Mirror TDD gate" sections), `docs/fleet-workflow.md` §9 (autonav-coverage caveat this closes
  for 2/3 addresses).
## Water-pump seesaw does not sink under Tomba's weight — the node[0x2b] contact producer never fires (kanban #8, 2026-07-21)
- **symptom:** grabbing the seaside water-pump seesaw while climbing does not tilt it; Tomba's weight has
  no effect. USER-reported on the live game (default config: pc_faithful + pc_render + pc_skip).
- **status:** ✅ FIXED AND CONFIRMED. USER verified on the live game 2026-07-22 ("it's already working
  now"). The defect was the signed-16-bit read of the ride/attach pointer — see "the defect" below.
- **repro:** `replays/bugs/seesaw-weight.pad` (6668 frames, from boot, no memory-card load). Replay
  headless, poll `padrec` until >=6400, `pause` — Tomba is frozen mid-grab.
- **the machinery (RE'd from a grab-state RAM dump, `PSXPORT_PAD_DUMP_AT=6600`):** the pump assembly is
  12 nodes with handler `0x8012EB54` (`beh_substate_edge_orchestrator`, native), `node[3]` = index 0..11.
  - `FUN_8012f494` (sub-state 0, the live one) holds an ANGLE DECREMENTER: `*(u16*)(*(u32*)(node+0xC4)+8)`
    minus 4 per frame, sign-extended past 0x800, clamped against `node+0x64`, stored masked to 0xFFF
    (a 4096-unit angle). Gated on `node[3]==2`. That is the beam tilt.
  - Escape from that sub-state is `FUN_80130788` returning nonzero; it dispatches on `node[0x29]` bits
    0/2/7. With both 0 and 2 clear it ALWAYS returns 0, so the object cannot leave the tilt sub-state.
  - `FUN_801308e0` (runs every frame) is gated entirely on **`node[0x2b]`**: if zero it returns
    immediately; otherwise it consumes the byte into `node[0x6c]`, indexes the `node+0xC0` pointer table
    with it, and sets **`node[0x48] = 0xe000`** (when `node[0x6c]==2`) or `0x2000` — the WEIGHT — plus
    `node[0x4e] = ±0x80`.
- **the measurement:** at the grab frame, all 12 nodes have `node[0x29]==0`, `node+0x44==0`,
  `node+0x48==0`, `node+0x4e==0`, and `node[5]==0` (tilt sub-state). Node index 2's tilt field already
  sits exactly at its clamp (`[c4]+8 = 0xF00`, limit `node+0x64 = 0xFF00`), so the decrement writes back
  the same value every frame — pinned at an end stop. A 30-frame step with the game verified advancing
  (other entities move) leaves all four nodes of the assembly BYTE-IDENTICAL.
- **the decisive datum:** `PSXPORT_WWATCH=800fb858,800fc3d0` over the whole replay records 6585 writes to
  each node's `+0x2b` — **every single one the value 0**, from clearing sites `0x80130D5C`, `0x801316CC`,
  `0x80146348`, `0x80084A80`. Across the entire session, including the grab, NOTHING writes a nonzero
  contact value into `node[0x2b]` for any of these objects. The consumer is healthy; the producer is
  absent. Every `+0x2b` write in the native `game/` tree is also a clear — the sole nonzero native writer
  is `interact_scan`'s activation (value 3), which is a different signal (activation, not contact index).
- **NEXT:** identify the guest producer that stamps a contact INDEX into a struck object's `+0x2b`. Lead
  (unverified): the player surface-probe cluster `FUN_80024448` / caller `0x8005D530` RE'd 2026-07-21
  (docs/findings/scene.md) "records the hit code" — a plausible home for the writer, and NOT yet ported.
  Confirm by watching `+0x2b` under `PSXPORT_GATE=1` for a nonzero write and its pc.
- **do NOT** poke `node[0x2b]` or `node[0x48]` to make the beam move — that fabricates the contact the
  real producer is supposed to supply, and hides the missing path.
- **refs:** `scratch/decomp/seesaw_leaves.c`, `scratch/decomp/seesaw_tier2.c` (13+3 fns decompiled from
  `scratch/bin/padram_6600.bin`), `game/ai/beh_substate_edge_orchestrator.cpp`, kanban #8.

### the defect (2026-07-21) — `Behaviors::areaSeasidePerframe` read the attach pointer as signed 16-bit
- **guest ground truth** (instruction stream at 0x80113CC0, not the decompiler, which rendered it as a
  pointer comparison): `lw v0,0x158(s1)` + `sltiu v0,v0,2` — a **32-bit UNSIGNED** test of G+0x158.
- **G+0x158 is the RIDE/ATTACH POINTER** — the object Tomba is holding (written by FUN_80057a68 as
  `player[0x158] = FUN_80024548(player,0)`), 0 when holding nothing. So the gate means "is the attach
  slot empty?" and is FALSE whenever he is attached (a pointer like 0x800FB960 is huge unsigned).
- **the port had `c->mem_r16s(0x800E7FD8u) < 2`** — signed 16-bit. `(int16)0x800FB960 = -18080 < 2` is
  TRUE, so `FUN_8011334C` fired in exactly the state the guest suppresses it. Unattached (slot 0) both
  agree — which is why it survived: the defect is invisible until something is grabbed.
- **fix:** `c->mem_r32(0x800E7FD8u) < 2u`.
- **A/B measurement (same replay, same binary, one line different), watching node[+0x48] (the weight):**
  - pre-fix: **0** nonzero writes across all 12 pump nodes over 6680 frames.
  - post-fix: **448** nonzero writes on node index 1 — the node G+0x158 points at.
- **★ WHAT THAT A/B DOES *NOT* SHOW (corrected 2026-07-21, same session):** all 448 writes fall in frames
  **315-748**. The grab is at ~frame 6400. So they are early-session activity and say NOTHING about the
  grabbed state. Worse, the post-fix dump at frame 6600 has `G+0x158 == 0` — the diverged replay never
  reaches a grab in its late portion at all. I originally reported this A/B as evidence that the fix
  "gates the weight path"; it is a true measurement that does not support that conclusion. The fix's
  effect on the SYMPTOM is unestablished, and the pad replay cannot establish it (the fix changes
  behaviour, so the open-loop input desyncs). Only a live retest can.
- **CONFIRMED BY THE USER (2026-07-22):** the seesaw sinks under Tomba's weight again. The fix was the
  whole bug. Note this closes the loop that headless testing could NOT: the fix changes behaviour, so
  the open-loop pad replay stopped reaching the grab entirely, and no amount of replay analysis could
  have confirmed it. `tools/drive_to_grab.py` exists for the next one.
- **Superseded by this confirmation:** the long `+0x2b` / contact-stamp investigation below. `+0x2b` is
  never stamped non-zero and that is evidently NORMAL — the weight arrives by another route. Do not
  resume the "find the missing +0x2b producer" thread; it was chasing a non-problem. The RE recorded
  there (consumer FUN_801308e0, the queues, the class-4 list) stays valid as reference.

### contact-stamp thread (2026-07-21) — queueing is fine; the value-2 stamp is unowned substrate
- The beam is **class 4, type 0, visible**, and IS in the class-4 queue (`ptr 0x1F80014C`, `count
  0x1F800152`; 11 entries, beam at index 5). Queueing is not the failure.
- `interact_scan` legitimately skips it — its activatable-type list has no type 0. That is the
  press-to-activate path (writes +0x2b = 3), NOT the weight path.
- The aux walk in `areaSeasidePerframe` consumes the **class-5 queue C** (`0x1F800154`/`0x1F80015C`,
  empty in the grab dump), so `FUN_80112A60` is not the beam's stamper. CORRECTION to an earlier note
  in this session: `0x80077F24/F30` are inside `FUN_80077EFC` (queue C), not `FUN_80077EBC`
  (`enqueueVisibleClass4`) — I mis-attributed them.
- Of the 105 `sb rt,0x2b(base)` sites with a non-zero source, **22 write the literal 2** (the weight
  case). The 13 MAIN.EXE functions containing them (`8001DC9C 8001E434 8001E860 8002313C 80023618
  80068A94 80068E68 80068FBC 80069948 8006B1FC 8006B390 8006B494 8006C0C4`) are **all unowned
  substrate**. So the stamp is original code, not a mis-port — the defect is upstream REACHABILITY of
  those functions. That is the next place to look.

### ★ THE GATE FOUND (2026-07-21): contact stamping is gated on QUEUE A, which is empty
- `FUN_80113700` — one of the three "trio" leaves `areaSeasidePerframe` dispatches — IS the contact
  stamper. Its whole body is wrapped in `if (_DAT_1f800144 != 0)`: **queue A's count**. Inside, it
  walks the object list at `0x800F2738` and, for each object with `obj[0]&1` and **`obj[0x2b]==0`**,
  walks queue A (`ptr 0x1F80013C`, `count 0x1F800144`) and dispatches per-type handlers
  (`8002192C`/`80020F7C`/`80021150`/`80021394`/`800216B4`/`80021AB0`) — the family that contains the
  `sb rt,0x2b` stamp sites, including the ones writing the literal 2.
- **In the grab dump `0x1F800144 == 0`, so `FUN_80113700` returns immediately and nothing is ever
  stamped.** That is consistent with every measurement in this entry: `+0x2b` only ever written 0.
- **Not a dump-phase artifact:** at the SAME instant the class-4 queue holds 11 entries (`0x1F800152`)
  while queue A (`0x1F800144`) and queue C (`0x1F80015C`) hold 0. All three are filled by the same cull
  pass, so an empty A alongside a populated class-4 queue is a real asymmetry, not a timing window.
- **NEXT:** find why class-2/9 objects are not being enqueued into queue A. Queue A is filled by
  `Cull::enqueueQueueA` (FUN_80077E7C) and by `Cull::enqueueByClass` (FUN_8007703C) for `obj[0xC]` in
  {2, 9} — BOTH NATIVELY OWNED (game/render/cull.cpp). That is the first natively-owned code on this
  path, and therefore the first real suspect. Note WWATCH cannot watch `0x1F800144` (scratchpad) —
  use the `.spad` sidecar or a live debug-server read.

### queue-A fill path is ENTIRELY ours — but the phase caveat blocks the conclusion (2026-07-21)
- All five writers of queue A (`ptr 0x1F80013C`, `cnt 0x1F800144`) are natively owned:
  `Render::objListWalk1` (0x8003BB50), `Cull::enqueueByClass` (0x8007703C), `Cull::decide` /
  `performBaseCull` (0x8007712C), `Cull::enqueueQueueA` (0x80077E7C), `Pool::initTypedPools`
  (0x800798F8). The per-frame filler is the base cull.
- **Our queue addresses are CORRECT** — verified against the guest instruction stream:
  `FUN_80077E7C` uses offsets 316/324 off `lui 0x1F80` (= `0x1F80013C` ptr, `0x1F800144` count, cap 24)
  and `FUN_80077EBC` uses 328/336 (= `0x1F800148`, `0x1F800150`, cap 40), matching `CULL_QPTR`/
  `CULL_QCNT`/`CULL_QCAP`. So `enqueueQueueA` is not mis-addressed. The separate populated 11-entry
  class-4 list at `0x1F80014C`/`0x1F800152` is a DIFFERENT structure (the one `interact_scan` reads).
- `Cull::decide` gates every queue assignment on `*(u32)0x1F800080 == 0`.
- **In the grab dump: `0x1F800080 = 0x467` (nonzero) and queues A/B/C all count 0.** That would mean
  the gate is shut and nothing is queued — but DO NOT conclude that yet.
- **★ PHASE CAVEAT (why this is not yet a finding):** the dump is taken at pad-service phase, and these
  scratchpad words are demonstrably reused there (`0x1F800084` reads `0x6C40`, neither a small cull
  mode value nor an object pointer). A/B/C reading 0 is equally consistent with "reset at frame start,
  filled and consumed inside the render window, dumped outside it". WWATCH cannot watch scratchpad
  (see tooling.md), so the queue counts cannot be sampled that way either.
- **RESOLVED — IT WAS A PHASE ARTIFACT (dead end, do not re-walk).** The `cullq` probe (registered
  channel, samples inside `areaSeasidePerframe` immediately before the trio dispatch) shows queue A
  holding **11-12 entries** in the overwhelming majority of 6582 samples, and the cull gate
  `0x1F800080 == 0` (open) in 6580 of them. Queue A is filled correctly; `Cull::decide` /
  `enqueueQueueA` are NOT the bug. The empty queues in the frame-boundary dump were exactly the
  scratchpad-reuse artifact flagged above.

### ★ THE REAL GATE (2026-07-21): the stamper's object-list head 0x800F2738 is NULL
- `FUN_80113700` has TWO gates, and it is the second that is shut. Disassembled prologue:
  `LH r2,0x144(0x1F80)` + `BEQ r2,r0` (queue A count — PASSES, 11-12 entries), then
  `LW r17,0x2738(0x800F0000)` + `BEQ r17,r0,+100` — if the object-list head is null the whole body is
  skipped. Ghidra's `pbVar1 = DAT_800f2738` is a genuine POINTER LOAD (verified in the instruction
  stream, since its typing had already misled twice).
- **Measured at the correct phase:** the `cullq` probe walks that head's chain (`+0x24` links) right
  before the trio dispatch — length **0 in 6570 of 6582 samples**. So the contact stamper runs and
  immediately returns, every frame. That is why `+0x2b` is only ever written 0, and why the beam never
  gets its weight.
- **Who fills that head:** `FUN_8007A810` (pool init — ZEROES it; reached from `Pool::init`, ours) and
  `FUN_8007AB44` (the registration, 2 store sites). `FUN_8007AB44` has 6 callers —
  `80068FBC 80069300 8006AE28 8006BB6C 8006C478 8006C59C` — **all unowned substrate**, and all in the
  same `0x80068-0x8006C` band as the literal-2 `+0x2b` stamp functions. One coherent contact/collision
  registration subsystem, entirely un-ported.
- **NEXT:** walk one link further up — find the callers of those six registrars and check whether any
  is natively owned. The first owned caller that fails to reach them is the defect. If they are all
  substrate too, the question becomes why the substrate never runs them (a missing dispatch upstream).

### ⚠ DOUBT CAST ON THE "CONTACT STAMPER" CONCLUSION (2026-07-21, same session)
- `FUN_8007A810` (pool init) sets the active-list head `0x800F2738 = 0` and tail `0x800F23A0 = 0`, then
  builds a FREE list of 4 nodes at `0x80100690` (stride 0x108, `node[0x28]=5`), free-head
  `0x800F273C`, free-count `0x800F2410 = 4`. `FUN_8007AB44` is the UNLINK/free path (doubly linked,
  prev `+0x20` / next `+0x24`) — it only ever REMOVES.
- **No code anywhere writes that head except those two.** Verified with a scan that understands both
  `lui`+store and `lui`+`addiu`-into-register forms, and `$gp`-relative stores are ruled out on range
  ($gp = 0x800BE0D4; reaching 0x800F2738 needs a 0x34664 displacement, far past the ±32 KB limit).
- **So the list starts empty and can only shrink — in the GUEST too.** Which means `FUN_80113700`'s
  outer walk is a no-op on the original hardware as well, and it is NOT the mechanism that gives the
  seesaw its weight. The earlier entry calling it "the contact stamper" identified a real code path
  (it does contain the `+0x2b` stamp sites) but almost certainly a VESTIGIAL one.
- **Consequence:** the branch from "FUN_80113700 is the stamper" onward is suspect, including the
  conclusion that its null list head is "the gate that is shut". The null head is probably NORMAL.
  What survives unharmed: the consumer RE (`FUN_801308e0` turns `node[0x2b]` into the `0xe000` weight),
  the measurement that `+0x2b` is never stamped non-zero, and the queue/class facts.
- **RESTART POINT for the next session:** find which of the 105 `sb rt,0x2b(base)` sites can fire for a
  **class-4, type-0** object, working from the object's own handler chain rather than from the aux/queue
  walks. Do NOT resume the 0x800F2738 thread.

### RE-DERIVED FROM A VERIFIED GRAB DUMP (2026-07-22) — the substantive facts hold
`scratch/bin/grab_prefix_6600.bin(+.spad)`, pre-fix build, asserted `G+0x158 = 0x800FB960` before use.
Replaces every reading taken from the overwritten `padram_6600.bin` (see tooling.md):
- **beam 800FB960 while genuinely held: `+0x2b = 0`, `+0x29 = 0`, `+0x48 = 0`, `+0x4e = 0`,
  `node[5] = 0`, tilt = `0xF47`** (pinned exactly at its clamp). So the central claim survives intact:
  with Tomba hanging on it, the contact byte is unstamped and the weight field is zero.
- **The beam IS in the class-4 list** — index 3 of 8 (`ptr 0x1F80014C`, `cnt 0x1F800152`), class 4,
  type 0. Correct queueing confirmed. (The contaminated read said index 5 of 11; the conclusion was
  right, the numbers were not.)
- **Every entry in that list has `+0x2b = 0`** — not just the beam. Nothing in the scene is stamped.
- Stamper outer-list head `0x800F2738` and tail `0x800F23A0` are both 0 here too, consistent with the
  pool-init reading that the list starts empty (so still no reason to believe that path is the
  mechanism).
- Queues A/B/C all read 0 at dump phase — unchanged, and still phase-limited; the `cullq` probe remains
  the only trustworthy sampler for those.

### ONE DEFECT, TWO BUGS (2026-07-22) — ⚠ HALF FALSIFIED THE SAME DAY, see below
The signed-16-bit attach-pointer read closed kanban #8 (water-pump seesaw not sinking) — that half
stands, USER-confirmed on the live game. It was ALSO credited with kanban #1 (jumping over an item
picks it up without touch contact); **that half is WRONG.** The attach-pointer gate only differs from
the guest while Tomba is ATTACHED to something, and a jump-over is precisely the unattached case, so
the mechanism could never have produced #1. The real cause of #1 is a different width defect, in
`ActorTomba::proximityCheck` — see "Jump-over pickup (kanban #1/#30)" below. #1 was reopened as #30
the same evening. Kanban #2 (bucket-pickup cutscene softlock) is NOT fixed by either and was closed
separately (delay-slot constant, below).

The transferable lesson survives and is the reason this entry stays: for any "an interaction resolves
when it should not / does not resolve when it should" symptom, look FIRST for a guest 16-bit quantity
ported with the wrong signedness. Three separate bugs in this codebase have now been exactly that.

## Jump-over pickup (kanban #1, reopened as #30) — a zero-extended gate ported sign-extended (2026-07-22)

**Symptom.** Jumping over an item collects it; the game requires touch contact. USER-reported twice
(#1, then #30 after #1 was closed on the wrong diagnosis above).

**NOT A REGRESSION.** `game/player/actor_tomba.cpp` has carried the defect unchanged since the repo
split (`git log -L` on the function returns only the import commit `ec7fe40`). #1 was never fixed,
only masked — while the bucket-pickup softlock (#2) was live the pickup sequence could not complete,
so the symptom was not visible; `f12ae00` fixed that softlock at 20:45 and the symptom came straight
back. Neither today's recompiler jump-table fix (`external c0caeef2`) nor the
`beh_prng_velocity_machine` rebuild (`f12ae00` / `0274c26`) is implicated: the rebuilt handler is
byte-identical to its gen body (2 MB RAM+spad A/B on `replays/bugs/bucket-softlock.pad`,
`PSXPORT_BEH_SUBSTRATE=80117658` vs native → `cmp -l` = 0 bytes).

**Root cause.** `ActorTomba::proximityCheck` (guest `FUN_80022060`, reached
`areaSeasidePerframe → interactWalk → proximityCheck`, which is what writes the item's
`node[4] = 2` = COLLECTED — confirmed live with a REPL `watch` + host backtrace on the item's state
byte). The guest's two accept gates both compare a **zero-extended** 16-bit quantity against a
sign-extended 32-bit limit:

    800220E0  andi a0, a2, 0xffff        ; isqrt XZ distance      -> slt (r_tomba + r_item), a0
    80022118  andi v1, v1, 0xffff        ; (tY-oY) + upT + upI    -> slt (dnT + dnI), v1

The port read both as `(int32_t)(int16_t)`. The vertical one is the bug: Y is DOWN-positive (measured
— a jump takes `*(s16)0x800E7EB2` from -1032 to -1285 and back), so once Tomba clears the item by
more than `upT + upI` the sum goes NEGATIVE. The guest's `andi` turns that into ~0xFF00 = a huge
positive and the `slt` REJECTS the touch — that is exactly what makes the vertical window one-sided
and stops a jump-over. Sign-extended it is a small negative that sails under the limit, so every
sufficiently-high pass over an item consumes it. Measured window on the bucket
(`upT+upI = 280`, `dnT+dnI = 410`): the guest accepts only `-280 <= tY-oY <= 130`, i.e. up to 280
above / 130 below; a full jump is ~370+ and therefore must miss. The distance gate is the same shape
— isqrt can return > 0x7FFF (XZ separation is a wrapping s16 pair, so up to 46340), and sign-extending
that turns a very distant object into a negative "distance" that passes the cylinder gate.

**Corroboration that it is the port and not the RE:** the sibling `ActorTomba::subHitboxCheck`
(`FUN_80022190`), same file, same guest idiom, already does it right —
`(int32_t)(uint32_t)(c->r[2] & 0xFFFF)` and `(int32_t)(uint16_t)vbandRaw`.

**Fix.** Zero-extend both gates; keep the sign-extended value only for the `0x1F80008C` scratchpad
store, which is the one place the guest itself sign-extends it (`sll`/`sra` at 0x80022134).

**Verified.** (a) genuine contact still collects — `replays/bugs/bucket-softlock.pad` still reaches
`*(0x800F16F0) == 2`; (b) the fix is inert where the defect never fired — 2 MB RAM dump at f1800 on
that replay is byte-identical pre/post fix (`cmp -l` = 0); (c) SBS `AUTONAV=combat` to f2500 shows the
identical pre-existing `0x801FE808..0E` stack residual from f210 with and without the change, so the
change is SBS-neutral (that residual is a separate, older issue).

**HONEST GAP.** The symptom itself was NOT reproduced headless: no route was found within this session
that gets Tomba jumping over a ground item in the aux list (every item the seaside routes reach is
either 800+ units above him or, like the bucket, snaps to his own position before it is ever tested).
The cause is proven from the instruction stream + the measured jump height + the measured accept
window, not from watching the symptom disappear. If it recurs, that is the first thing to build.

## Bucket-pickup cutscene softlock (kanban #2) — the delay-slot branch constant (2026-07-22)

**Symptom.** Picking up the bucket under the default config (pc_skip=true) softlocks: Tomba freezes,
scratchpad cut-mode `0x1F800137` latches at 1 and never clears. Repro
`replays/bugs/bucket-softlock.pad` — position `0x800E7EAE` stops changing at f500 and is still frozen
at f3000 (replay ends f1764).

**Root cause (FIXED).** `beh_prng_velocity_machine` (FUN_80117658, the seaside collectible Tomba takes
the bucket from) transcribed the wrong constant into one branch. At `0x80117be0` the guest runs
`beq v1,v0` comparing the object's `phase` byte against **v0 = the variant byte loaded at
0x80117ba0**, which is 1 on that path. The transcription read v0 as `3` — the value `0x80117bd4`
loads, which is the DELAY SLOT of the *other* branch's `j 0x80117cc0` and never executes here. Effect:
`phase` latched at 1 forever, so the `FUN_8005308C` wait never ran, the end-of-pickup script
(`FUN_80040CDC`) never started, and cut-mode stayed 1 = the softlock.

This is the same defect CLASS as the seesaw/pickup bug above (a guest register test transcribed with a
wrong width/value). Suspect it first for any "state machine never advances" symptom.

**Verified effect of the fix.** The pickup machine now walks HANDOVER -> AWAIT_PLAYER -> AWAIT_SCRIPT
and the bucket cutscene script actually runs: `PSXPORT_DEBUG=pickup` shows the cursor advance
`80148574 -> 8014857C -> 80148584 -> 80148594 -> 8014859C -> 801485A4`, and the script parks on op
0x01 (`FUN_80041D60` = spawn a sub-actor and wait for its state byte to reach 2), having spawned the
message box. **The cutscene still does not COMPLETE** — see the open residual below.

**FALSIFIES the earlier card note "FUN_8007D594 is never called / DEAD END".** That measurement was
taken on captures in which the cutscene never started (because of this very bug), so of course the
dialog machine was cold. With the fix, `PSXPORT_DEBUG=ovhit` reports
`0x8007D594 DialogBoxSm::step : native=963 oracle=0` on the same replay. The dialog state machine IS
the live driver of this cutscene. Do not re-derive it as cold.

**OPEN residual — where this now stops.** The message-box actor is `FUN_8007DA50` (UNOWNED, and NOT in
the card's previously-surveyed dialog set); its state 1 calls `FUN_8007D594` = the ported
`DialogBoxSm::step`. On the repro the box node `0x800EED90` sits at `[4]=1 [5]=2` for 1464+ frames and
does not advance on X taps, so the script's wait for `[4]==2` never satisfies. Next step is
`DialogBoxSm::step` under exercise (it is port_check-PASS but was never exercised until now) and
`FUN_8007DA50` itself. Chain from the pickup down to the box is now fully owned/RE'd.

**Tooling added.** `tools/find_refs.py <dump.bin> <addr> [--rw r|w]` — "who reads/writes guest address
X", scanned straight off the instruction stream of a RAM dump (Ghidra's Reference DB misses the
`lui`+offset absolute-global form this codebase lives on, and `decomp.sh` has no xref mode). It is
what located the cut-mode writers and the popup-node users here. `PSXPORT_DEBUG=pickup` traces the
collectible's script progress. **FOLLOW-UP:** the `pickup` channel still needs its one-line entry in
`external/psxport/docs/config.md` — the edit was made and then backed out because that submodule was on
a detached HEAD carrying another agent's in-flight change, and committing there would have moved the
pointer under them. Add it when the submodule is quiet.

**Method note.** A pad replay is only valid for the timeline it was recorded on: this capture does NOT
reproduce under `PSXPORT_PC_SKIP=0` or under the oracle (`PSXPORT_GATE=1`), because those reach
free-roam ~800 frames later and the same button stream drives Tomba somewhere else. Confirmed by
running both to f4000 with a cut-mode watchpoint — the pickup never happens. Do not treat "the oracle
does not reproduce it" as evidence about the bug.

## beh_* rebuild A/B campaign (kanban #10) — coverage, the instrument, and 1 root-caused divergence (2026-07-23)

- **task:** the 65-entry native behaviour table (`game/object/behavior_dispatch.cpp`) is REBUILDS, not
  transcriptions, so `port_check` cannot gate them. Two shipped user-visible bugs came from a silent
  rebuild divergence (#2 `beh_prng_velocity_machine`'s hardcoded 3 taken from the delay slot of the
  other branch's `j`; #1/#30 `proximityCheck`'s sign-extend where the guest `andi` zero-extends).
  This campaign measured coverage, then differentially A/B'd every REACHED handler.

- **THE INSTRUMENT — `PSXPORT_MIRROR_VERIFY` IS NOT A VALID GATE FOR A beh_* REBUILD (measured, not
  argued).** Its leg 2 sets `verify.inSubstrateLeg`, which suppresses the WHOLE override registry, so
  every nested native engine class differs between the two legs. Control run on
  `replays/bugs/bucket-softlock.pad` with BOTH legs forced to the substrate for the armed handler AND
  every beh_* native disabled: **37167 mismatches for 0x80124E74 alone** (hi/lo from a multiply, ~14k
  object bytes at 0x800Fxxxx). A green MIRROR_VERIFY on a beh_* means nothing; a red one is
  unattributable. (The kanban note claiming a clean `=all` survey over this capture is falsified.)
  Also: `strictArmed` parses its list with `strtoul(..., 0)`, so a bare `8002918C` is read as DECIMAL
  and the run silently checks NOTHING — the address list MUST be `0x`-prefixed.
- **The valid instrument is END-STATE comparison at a rendezvous:** run the whole scenario twice,
  swapping only the handler under test (`PSXPORT_BEH_SUBSTRATE=<addr>`), and diff the 2 MB RAM dump
  PLUS the `.spad` sidecar. Nested overrides are native in BOTH runs, so they cancel. Determinism of
  the rig was verified first (two independent runs of the same config: 0 differing bytes, RAM + spad).
  Tooling: **`tools/beh_ab.sh`** (`cov` / `base` / `one` / `sweep`) + **`PSXPORT_DEBUG=behcov`**
  (per-handler reach counter) and **`PSXPORT_BEH_TRACE=<addr>` + `PSXPORT_DEBUG=behtrace`**
  (per-invocation node-byte delta logged at the DISPATCH SITE, so the two legs' logs are line-aligned
  and `diff` names the first divergent invocation/object/field).

- **COVERAGE (measured, `behcov`, per replay — reached / 65 table entries):** bucket-softlock 53,
  save-sign-softlock 54, seesaw-weight 54, weapon-impact-bucket 54, house-on-the-point 53,
  hut-entry-door-freeze 53, boot-smoke/short-session 4. **Union: 54 reached, 11 never reached.**
  Most reached handlers fire 1000+ times on the bucket capture (`substate_edge_orchestrator` 10000+;
  `seaside_prox_substate` only 1).
  **The 11 with NO scenario in the replay library** (all cutscene-script driven — the library has no
  A06/A08 cutscene capture): `0x80041098 script_interp_step` (its BODY is exercised — `beh_sop_intro_pilot`
  and `beh_a06_scripted_actor` call `ScriptInterp::step` directly — but the TABLE entry never dispatches,
  so `PSXPORT_BEH_SUBSTRATE` cannot swap it; it needs a different A/B), `0x801189E8 a06_multi_actor`,
  `0x801280D0 a08_scene_actor`, `0x8013AA14 a06_scripted_actor`, and the seven op-0x3E fade/script
  fnptrs `0x80139728 / 0x8013AEF0 / 0x8013AFD8 / 0x8013B074 / 0x8013B178 / 0x8013B274 / 0x8013B29C`.
  **These need an A06/A08 cutscene replay capture before they can be verified at all.**

- **ROOT-CAUSED + FIXED — `ActorZonedAttacker::defaultSubStateMachine` (0x80143A00): the node[5]
  jump-table arms were SHIFTED BY ONE.** Found because forcing `beh_id_compare_motion_dispatch`
  (0x80145230) to its gen body changed 148 bytes of object state on the bucket capture (first at
  f421); `PSXPORT_THUNK_FORCE_GEN` bisected the 6-address ActorZonedAttacker cluster to this one
  address (forcing it alone reproduced the same 148 bytes; the other five were 0).
  The real jump table (read out of the running game at **0x8010A1EC**) is
  `[0]=80143A50 [1]=80143A68 [2]=80143BC8 [3]=80143D84 [4]=80143C78 [5]=80143C00 [6]=80143F24 ...`
  — entry [0] is NOT an alias of entry [1]: it clears `node[0x62] &= ~4` and re-packs
  `node[4..7] = 0x00000101` as one word, and only then falls into [1]. The rebuild collapsed the two
  into a single `case 0`, which dropped both writes AND slid every arm below: `node[5]==1` ran entry
  [2]'s body, `==2` ran [3], `==3` ran [4], `==4` ran [5] (arms 6..15 were never shifted). Symptom at
  the divergence: an actor in move-id 1 called `0x801425F0` instead of `0x80142788`, so it entered a
  different attack arm — `node[7]` 1 vs 2, cooldown `node[0x40]` 44 vs 60, heading `node[0x38]`
  0xD374 vs 0xD3F0, which then propagated into `Animation::applyFrame` positions.
  **Gate: `PSXPORT_THUNK_FORCE_GEN=0x80143A00` on bucket-softlock f2100 → 0 differing bytes (RAM +
  spad), was 148.** Two further fidelity defects were found in the same body while reading it against
  `ov_a00_gen_80143A00` and fixed in the same pass: case 0xC compared `(int16_t)v0 < 800` where the
  guest `slti` compares the FULL 32-bit v0 (a distance in 0x8000..0xFFFF read as negative), and its
  near branch jumped to 0x80144550 (which first tests v0) instead of the guest's 0x80144558 (the
  unconditional `node[6] = 3` store).

- **FIXED — `beh_id_compare_motion_dispatch` (0x80145230) had no guest stack frame.** `abi_extract
  --contract` says frame=56 with s0..s4+ra spilled at sp+32..52; the native descended nothing. Not
  bookkeeping: its SPAWN block hands `FUN_8003116C` a pointer to an arg struct built at `sp+0x10`, so
  the struct was assembled 56 bytes high, over the CALLER's live locals, and the fields the callee
  reads but this code does not write (+0x10/+0x14/+0x18) came from unrelated data. Mirroring the frame
  removed ALL residual stack churn between the legs (frames 411-420 went from 5-16 differing bytes to
  0), which is what made the remaining 148-byte object diff cleanly isolable.
- **FIXED — `beh_actor_move_sm` (0x8011D988) called `Actor::boundsCull` instead of dispatching
  0x8007778C.** `Actor::boundsCull` is a separate rebuild that inlines `performBaseCull` rather than
  dispatching 0x8007712C, so the handler took a different cull path from the guest body it replaces —
  the dispatch trace showed the gen leg making a `Cull::cullWrapper ra=8011D9F4` call this leg never
  made. Now routed through the registry with the RE'd ra constants (`guest_fn(c, 0x8007778C,
  0x8011D9F4/0x8011DBA4, nd)`), per CLAUDE.md's one-registry rule. (This did NOT close its end-state
  diff — see OPEN below.)

- **A/B RESULT — bucket-softlock (f2100, 54 reached handlers, RAM+spad): 52 of 54 are 0 differing
  bytes** (final binary, after the fixes below), i.e. the rebuild is equivalent on every path that
  capture executes. `0x80145230` went 148 -> 0 with the ActorZonedAttacker fix.
  Residual on that capture: `0x80124E74 beh_jumptable_release_trigger` = **1 byte** at 0x800BF82C, and
  it is NOT a rebuild slip — the store is the SAME site in both legs (`gen_func_8007074C` writing the
  3-word block at 0x800BF824/28/2C from `gen_func_800424F0`), and the differing byte is the LOW half of
  a `coord << 16` word that the guest's own opcode handler reads from an UNINITIALIZED stack slot
  (0x800424F0 writes only sp+26/30/34 and then loads sp+24/28/32 as words). The value is guest stack
  history, so it moves with any legal difference in stack churn. Watchpoint evidence:
  `scratch/logs/beh_ab/cw_forced.log` vs `cw_native.log` (native 0x08000000, gen 0x080000C0).

- **OPEN — `0x8013C9C0 beh_scatter_ramp_machine`, 100 bytes on bucket-softlock (0x800D28AE region).**
  Localised, not root-caused: `behtrace` shows the FIRST divergent invocation is f160 on objects
  0x800EE488/0x800EE510, where the native writes **`node[1] = 1` and the guest body does not** (every
  other node byte in that invocation is identical). No write to `obj+1` exists in
  `game/ai/beh_scatter_ramp_machine.cpp`, so it comes from a callee the native reaches and the gen does
  not: the dispatch trace forks at the same point, native into
  `leaf_80024F18` / `Engine::fieldTargetCursor` / `Animation::advanceLinkChain`, gen into two
  `Math::isqrt16` calls. Next step is that call site.

- **OPEN — 2 handlers still diverge on `replays/bugs/house-on-the-point.pad` (f4500):**
  `0x8011D988 beh_actor_move_sm` (2071 bytes) and `0x80121978 beh_id_routed_dispatch` (30 bytes).
  Both diverge FIRST at the graphics-record freelist cursor **0x800E7E74** and its count **0x800ED098**,
  then in the record bytes at 0x800ED7xx — i.e. the two legs bind a DIFFERENT NUMBER of graphics
  records. For 0x8011D988 the per-invocation node deltas are IDENTICAL across all 543 invocations
  (`behtrace`) and, after the cull-routing fix above, the registry dispatch stream is identical too —
  so the remaining difference is in a native that is not registry-dispatched (an inline `eng(c).*`
  call) or in the record-allocating callee's arguments. NOT yet root-caused. Note the same replay is
  kanban #47's state-corruption repro, so these may be the same defect seen from the other end.
