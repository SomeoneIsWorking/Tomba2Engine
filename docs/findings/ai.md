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
- **status:** A REAL PORTING DEFECT FOUND AND FIXED on the path (2026-07-21); whether it is the WHOLE
  bug is UNCONFIRMED — needs a live check by the user. See "the defect" below.
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
- **NOT VERIFIED:** that the beam visibly sinks. The fix changes behaviour, so the open-loop pad replay
  diverges after it and no longer reaches the grab at the same frames; a fixed-frame visual compare is
  meaningless. Needs a live confirmation. Also still true post-fix: `node[+0x2b]` is never stamped
  nonzero, so the FUN_801308e0 contact path remains unexercised and the 0x48 writes are arriving by
  another route. If the seesaw still misbehaves live, that is the next thread.
