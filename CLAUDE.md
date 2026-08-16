# Tomba2Engine — a PC-native reimplementation of Tomba! 2's engine

**Unlabeled content is machine convention, revisable by any session. USER lines are verbatim dated
quotes and only those.**

**Goal:** REBUILD Tomba! 2 as a self-contained **PC-native game engine** in C++, running the real game
content. Not an emulator, not a recompiled-MIPS blob with I/O bolted on. Effort/time is not a constraint.

> USER, 2026-06-14: *"new direction — port to PC, no PSX emulation, no PSX BIOS."*
> USER, 2026-06-14: *"make the game itself do PC native rendering instead of PSX emulated rendering."*
> (both `docs/journal.md`, sections "later 9" / "later 23")

## The 5 paths — canonical vocabulary (read this first)

Two execution paths and two rendering paths. Any run is one exec × one render.

**Execution**

- **recomp_path** = `PSXPORT_GATE=1` — the full static-recomp substrate (`generated/shard_*.c`) driving
  gameplay under a native boot+frame loop. Not strictly PSX-faithful (async→sync conversions, PC-native
  CD/file I/O). **Runs perfectly.** THIS IS THE ORACLE for byte-comparison.
- **pc_faithful** = default (`./run.sh`, no flags) — supposed to be a byte-exact clone of recomp_path in
  clean OOP. Currently BROKEN. Making it byte-exact to recomp_path is **Job #1**.
- **pc_skip** = a **PER-FORK bool** (`Game::mPcSkip`), NOT a third path. Every collapsed multi-step init
  is a fork:
      if (game->mPcSkip) load_in_one_step();          // shortcut — end-state only
      else               load_in_multi_step_faithfully();  // MUST byte-match recomp_path
  Default uses `mPcSkip=true` (shortcut where forks exist). SBS forces `mPcSkip=false` so the faithful
  branch can be byte-compared.

**Rendering** (orthogonal to exec)

- **psx_render** = `PSXPORT_RENDER_PSX=1` — the substrate's GTE + OT + GP0 renderer.
- **pc_render** = default — native renderer (rules: USER 2026-07-07), and a READ-ONLY OVERLAY: the PSX render path still EXECUTES
  underneath (its guest-memory operations — packet pool, OT, libgs state — are part of the faithful
  byte-exact state); pc_render produces the PICTURE from its own pass, bypassing GTE/OT/PSX render
  subsystems for DRAWING only. It reads guest RAM + PC engine classes (e.g. fade state from the fade
  engine) and writes ONLY its own host memory — **any guest-memory write from pc_render is a bug** and
  will surface as an SBS diff. Render bugs are EXPECTED, and fixes DEFERRED until pc_faithful is
  recomp-identical.

**Combinations**

- **`PSXPORT_ORACLE=1` IS THE REFERENCE BUILD.** Implies GATE + RENDER_PSX and additionally forces pure
  OT painter order, so no native band/depth/widescreen/fps60 decision can reach the picture. Reach for
  this whenever the question is "what does the substrate actually draw".
- `PSXPORT_GATE=1 PSXPORT_RENDER_PSX=1` — recomp_path + psx_render (skips OP.FMV, fix deferred). It walks
  the guest OT but still hands every prim to the NATIVE render queue's layer split + per-pixel depth, so
  it is **not** the pure reference — its fidelity depends on pc_render producers having run, which on
  this leg they have not (kanban #78: that cost a whole investigation).
- `PSXPORT_GATE=1` — recomp_path + pc_render. Works, known rendering issues (deferred).
- `./run.sh` — pc_faithful + pc_render. Currently broken. Target: byte-exact to recomp_path.

**Enhancements — the third behavior class (USER 2026-07-16).** **pc_enh** = `PSXPORT_ENH=<name,name|all>`: deliberate,
MEANINGFUL guest-state changes on top of the faithful engine (planned: expanded object load/unload,
faster fades/transitions). One name per enhancement + `all` umbrella, gated via `cfg_enh("name")`,
registered in `external/psxport/docs/config.md`, and force-suppressed under `PSXPORT_ORACLE`/SBS inside
`cfg.c` so byte-compares stay enhancement-free by construction. Contrast: pc_render never writes guest
memory; pc_skip changes no meaningful end-state; pc_enh is the only class allowed to change what the game
does.

**SBS.** `PSXPORT_SBS_MODE=full` — two `Game`s in one process, core A = pc_faithful, core B =
recomp_path, byte-comparing guest RAM step-for-step with `mPcSkip=false` on both. Divergences are FATAL —
no allowlist, no residual list, no "known diff". A diff means pc_faithful is wrong (usually) or
recomp_path is wrong (rare — recomp has its own bugs; validate against the running game).

**BUT core B IS NOT AN INDEPENDENT ORACLE, and the USER is right that it should be.** USER, 2026-08-12:
*"oracle compare should be done against a verified emulator like beetle imo, not our unverified
'faithful' path"*. Both cores are OUR code, written from the same reading of the same disassembly, so a
shared wrong assumption reads as SUCCESS — the architecture-level form of an instrument that cannot show
the other answer. The MECHANISM is sound and stays: byte-exact, lockstep, no allowlist, and its
`PSXPORT_SBS_CANARY` self-test demonstrably works (measured 2026-08-12: canary at f60 tripped on the
exact address, after identical at f0/f30). What changes is the REFERENCE. Plan, with the measured
feasibility and the one hard part (we HLE the BIOS, Beetle executes it):
`external/psxport/docs/plans/oracle-against-beetle.md`. Until that lands, read a green SBS run as
"our two implementations agree", never as "the port is faithful".

**Job #1 — right now.** Run SBS full, find the first pc_faithful divergence, root-cause it, fix. Repeat
until zero-diff. Rendering bugs are deferred UNTIL (a) zero-diff is reached, OR (b) an SBS diff traces
back to pc_render writing to guest memory.

## Framework-owned rules — read them there, not restated here

`external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` are the authority for the
shared rules; the workspace map is `external/psxport/docs/workspace/WORKSPACE.md`. In particular:

- **THE PICTURE COMES FROM GAME STATE, NEVER FROM WHAT THE GTE PRODUCED** (USER, 2026-07-23 →
  2026-08-06) — `PROTOCOL.md` holds the
  binding statement, its two checkable rules, and the USER quotes behind them.
- **BREAK FIRST, THEN REBUILD** (USER 2026-07-16; generalizes the 2026-07-15 render directive) —
  delete the transitional mechanism, let the gap be honestly visible,
  then build the native producer; never keep the stopgap alive alongside the replacement.
- **Never edit `external/psxport`** — read-only pinned consumer, and `run.sh` re-syncs it to the RECORDED
  gitlink every run, so an edit here is liable to be silently reverted mid-gate. Framework edits happen
  in the workspace dev clone (`$PSX/psxport`, i.e. `../psxport`) only. To build against in-progress
  framework work without touching the submodule: `PSXPORT_DIR=$PSX/psxport` (env or `-D` to cmake); it
  defaults to the submodule so a bare clone still builds standalone — keep it that way. `run.sh`
  announces which checkout a run used and whether it was dirty: read that before trusting a measurement.
- **Concurrent sessions: a full clone per session, never a `git worktree`, never `git stash`** — a
  worktree shares `.git`, so `refs/stash` and `.git/modules` are common ground; a stash-pop has already
  grabbed another agent's work in this project.
- **Never write run artifacts to `/tmp`** — use this repo's gitignored `scratch/`, split by kind.

**The Tomba-specific evidence behind the GTE rule — do not lose this.** Inverting composed GP0/OT output
to recover a transform leaves a residue that is A FUNCTION OF THE CAMERA, measured on this game:
**0.13 px camera-still, 1.53 px panning, 12/12 sign alternations, net drift −1.23 px, X 1.53 px vs Y
0.43 px** — a "vibrating" layer nothing in the game moves (full measurement, legs and negative control:
the "mean |dX|" entries in `docs/findings/render.md`). So Dusklight may lerp recorded matrices and we may
not — theirs are FLOATs from a decomp, ours s16 GTE output; resolve from the SUBMITTER (the
`SetRotMatrix`/`SetTransMatrix`/RTPS inputs) and we are in their position. PSX having no Z-buffer is a
SEPARATE fact (`OTZ` is a bucket index, not a distance) and argues FOR native per-vertex depth.

## WORKFLOW FIRST (outranks the task)

A workflow defect — re-deriving a solved bug, an unsearchable doc, untracked env-var sprawl, a bloated
CLAUDE.md — takes PRIORITY over the task in hand. Fix it, then resume. **Knowledge lives in structured
docs, NOT in this file.** Improve the doc/tool/workflow when it falls short, same session.

- **BEFORE investigating any bug**, search `tools/findings.py <symptom words>` (curated
  `docs/findings/<subsystem>.md` + raw `docs/journal.md`); read `docs/findings/INDEX.md` at session start;
  promote useful journal hits. **AFTER a durable fix**, record it in `docs/findings/<subsystem>.md`
  (symptom / status / cause / fix / refs — DEAD ENDS too) and regen the index.
- **Live bug list:** `tools/kanban.py` (skill `bug-tracker`) over `docs/kanban/` cards
  (backlog|todo|doing|done). Card when the USER reports a symptom, `move <id> doing` when chasing,
  promote to findings + `done` on confirm. Evidence images in `docs/reference/issues/`.
- **The TRACKING STACK — four orthogonal maps, one question each.** Consult at task start, update in the
  SAME commit as the work.
  - **codemap** (`tools/codemap.py` → `docs/code-map.md`) — WHERE is it: guest addr → native owner,
    LIVE/ORPHAN, auto-scanned from source. **Run `--addr <hex>` before reimplementing any `FUN_xxxx`:**
    natives are indexed by guest address AND by override INSTALL SITE, so it warns ⚠ DUAL-OWNERSHIP,
    ⚠ CLAIM-WITHOUT-INSTALL (two files claim one address, one installs) and ⛔ DELIBERATELY ABSENT
    (`docs/port-map.md` says the layer was removed on purpose). `--conflicts` lists every duplicate-owned
    address (how FUN_80040B48/80040CDC got duplicated); `--selftest` (in `tools/precommit_gate.sh`) proves
    the index still answers positively for every ownership shape it claims to cover.
  - **port-map** (`tools/portmap.py` → `docs/port-map.md`) — IS it ported, and REAL not a HACK:
    `verified | ported-unverified | hack | todo | blocked`. `next` = the next RE-ready step (work THAT,
    not a downstream one); `hacks` = the debt list, kept shrinking (a hack MUST name its real fix + death
    condition); `check` flags jumped-ahead work. `docs/port-progress.md` is the DETAIL behind it — the
    boot→gameplay spine and per-function status; port top-to-bottom.
  - **parity-map** (`tools/parity.py` → `docs/parity-map.md`) — IS it SBS byte-exact (Job #1): per unit
    `verified` (cite frames+gate+evidence) | `diverges` (a live Job#1 bug — `check` FAILS) | `partial` |
    `untested` | `n/a` (pc_render overlays never write guest RAM). Record every SBS 0-diff:
    `parity.py set <unit> --status verified --frames N --gate '<cmd>' --evidence <commit>`.
  - **behavior-map** (`tools/behavior.py` → `docs/behavior-map.md`) — WHAT it DELIBERATELY changes vs
    recomp_path, so a byte diff triages instantly as bug-or-intended. Primary axis = GUEST-MEMORY AFFECT:
    `none` (host-only overlay — a guest write is a BUG) | `non-canon` (writes guest RAM but byte-matches
    at the rendezvous — pc_skip) | `full` (deliberately changes canon state — pc_enh; MUST be
    force-suppressed under ORACLE/SBS or `behavior.py check` FAILS). Register every enhancement here.
- **What is MISSING from the picture:** `docs/unported-render-inventory.md` — the ranked cross-cut list of
  visual layers pc_render does not natively produce, each with its guest producer, dependency order and
  user-visible cost. None of the four maps can answer this (a deliberately deleted layer looks identical
  to an unported one in the codemap). Read it before any "X is missing under pc_render" work.
- **Areas:** `docs/areas.md` — the 22 areas (0..21) and how to reach one (cold `warp` is broken; use the
  settled recipe). NAMING RULE: an area index is a fact, an area name is a claim needing a source (USER
  or guest data) — two cards were filed against invented names before this existed.
- **Other docs:** `external/psxport/docs/faithful-execution.md` (HOW pc_faithful achieves byte-exactness — guest-stack
  residency, native fibers, ported scheduler primitives; read before touching any faithful path),
  `docs/fleet-workflow.md` (operator + subagents at scale), `docs/engine_re.md`, `docs/render-arch.md`
  (VK renderer), `docs/gfx-debug.md` + skill `gfx-debug` (render bugs),
  `external/psxport/docs/config.md` (cfg module — no raw getenv), `docs/driving-the-game.md` (REPL +
  scenario replays), `replays/README.md` (deterministic pad-capture library — reproduce bugs headless
  without live input), `docs/project-map.md` (build).

## RE first — never black-box debug

At any diff-hunt trigger (magic offset, `sub_XXXX`, taxi free-fn, mystery `obj[+0xNN]`), PAUSE and RE the
surrounding function into named struct types + owned methods FIRST. Mechanical fold-in is only for
already-RE'd code. RE default = **Ghidra headless** (`external/psxport/tools/decomp.sh`) — never `external/psxport/tools/disas.py` to
understand behavior, never hand-walk backwards through addresses; `disas.py` is single-instruction
spot-check AFTER Ghidra only.

## Core directives

- **Full native ownership is always the answer.** No engine-vs-content fence. Enemy AI, physics, quests,
  game rules — all portable native. Un-ported code runs as the substrate (`func_<addr>`). When in doubt,
  own it. (Source, verbatim but undated: `docs/journal.md` "later-115" — *"the next step is full ownership
  of the game engine, no faking anything, respawn when you need to."*)
- **ONE behavior, no env-gate.** Never a PSXPORT_* toggle to A/B a new native path against the old one —
  make the new path THE path. Legacy `*_RECOMP` / `FAITHFUL` behavior-flags are scaffolding to retire.
  Diagnostics (`PSXPORT_DEBUG=chan,chan`) are fine, but every channel is registered in `cfg` and
  documented in `external/psxport/docs/config.md`; never raw `getenv`.
- **Engine owns the game; NO PSX intricacies leak in.** Own the world, objects, camera, projection and
  render ordering. Never read/honor/reproduce OT / draw order / GTE output / GP0 packets / disp-env. If
  you are reasoning about GP0 to explain something, STOP — rebuild from GAME STATE.
- **REBUILD, don't transcribe.** A native fn reproducing PSX instructions/packets byte-for-byte is
  PSX-simulation. Match the observable RESULT (world it builds, picture it draws, interface state content
  reads back), not the PSX mechanism.
- **FAIL-FAST.** All I/O and timing PC-native + synchronous. Any PSX async/wait (VSync waits, CD
  command-waits, async CD reader, GPU/MDEC DMA timeouts, IRQ loops) must be done inline or ABORT with a
  diagnostic. Never re-introduce instant-VSync / fake-CD bandaids or env escape hatches.
- **No bandaids / no magic constant offsets.** Name the root cause; every lifted fn / patched value gets
  RE justification. Especially **no residual RAM diverges** — no SBS diff may be waved off as
  known/expected/residual. Only exception: memory the still-recomp side never reads.
- **One override registry, one dispatch point (USER 2026-07-16).** A native engine class is wired by guest address in THE
  registry — `overrides::install(addr, name, native, gen[, setter])`
  (`external/psxport/runtime/recomp/override_registry.h`) — and then EVERY caller, substrate included, reaches the native
  method; so a leaf engine (e.g. the fade engine) can be owned globally on its own, with no "contiguity
  required" precondition. ONE entry per address holds `{ native, gen }`; one dispatch decision runs `gen`
  on the oracle leg (core B / `psx_fallback` / `verify.inSubstrateLeg`) and `native` everywhere else,
  shared by both interception points — the `g_<mod>_override[]` thunk (direct `func_X(c)` calls) and
  `rec_dispatch`. `gen` is the recompiled body (`gen_func_`/`ov_<tag>_gen_`); `gen == native` expresses an
  oracle-allowed primitive (scheduler leaves that must fire on both cores). `setter` = the module's
  `shard_set_override`/`ov_<tag>_set_override` to intercept direct callers too, or `nullptr` for
  rec_dispatch-only wiring. Handlers use guest ABI (args in r4..r7, ret in r2) and must byte-match the
  substrate body — SBS gates it. No separate register_/traceHit table, no hand-rolled `psx_fallback`
  guard: the gate lives in ONE place (`PSXPORT_DEBUG=dispatch`/`ovhit`). PlatformHle stays the separate
  BIOS/hardware-sync HLE table (raw `shard_set_override`, fires on both cores). **No interpreter
  fallback:** a `rec_dispatch` miss aborts with a backtrace — fix it by seeding the recompiler, porting,
  or wiring an override.
- **WRAP THE GUEST-MEMORY SOUP — ported bodies must READ as game code.** A function full of
  `c->mem_r32(0x800E7FD8)` and `mem_r8(node + 0xB)` is a transcript, not a port, and it actively HIDES
  bugs: you cannot see a state fork that never fires when every read is an opaque hex address. (The
  seesaw bug was exactly that — a `mem_r16s` where the guest compares 32-bit unsigned — and it became
  findable only once the surrounding code read as state.) So when you touch a body: typed struct LENSES
  over the guest blocks (`dlg.state()`, not an offset), NAMED constants for every literal address saying
  what it IS, enums for state-machine states, method names that say what the code DOES, ABI plumbing via
  `external/psxport/runtime/recomp/guest_abi.h` rather than open-coded `r[]` juggling. Byte-exact mechanics STAY
  byte-exact; this is about how they read. **Exemplars — match them rather than inventing a style:**
  `game/ui/panel_fill.cpp` (converted from a `port_gen` transcription: named packet layout, decoded
  attribute bits, a table replacing the guest's jump table) and `MusicCoord::voiceMixTick`. **The checker
  trap that keeps bodies unreadable:** `port_check` compares STATIC store sequences, so a genuine rebuild
  can FAIL it by construction (a table replacing an unrolled jump table has fewer store sites). Then
  prove equivalence the way panel_fill.cpp does — run the repro with the native installed and with it
  disabled so the gen body runs, diff the 2 MB dumps, cite the result. Do NOT contort readable code back
  into a transcription to satisfy the checker, and do NOT skip the proof either.
- **REAL C++ CLASSES, no `extern "C"` shims.** Subsystems are instance methods on Core-owned classes
  (`c->engine.foo.method(args)`), pure math/utility static (`Math::rotmat(c, a, b)`). No `ov_*` free
  functions, no `foo_impl` helpers under `Class::foo` wrappers. When in doubt, INSTANCE.
- **No file-scope globals.** No `g_*`, no non-const file-scope statics anywhere in `game/` or the framework's
  `runtime/recomp/`. Everything a real class with a header, state on `Game`/`Engine`/subsystem.
- **No inline local `extern` decls (USER 2026-07-17).** Declare functions in the owning header and `#include` it; NEVER
  `{ int foo(Core*); … }` inside a block to reach a function defined elsewhere (legacy instances exist —
  add none, convert the ones you touch). Name things meaningfully, no doubled/placeholder names like
  `gpu_gpu`. Lift repeated sub-expressions into named locals. Match the surrounding file's idiom.
- **MIRROR THE GUEST STACK — never revert/exclude a leaf because it pushes a frame.** If the substrate
  body of a leaf you're owning descends `sp` (`addiu sp,-N` + register spills), the native port MUST
  reproduce that frame: `c->r[29] -= N` at entry, the callee-save spills (`ra`/`s0..s3`) written at their
  RE'd offsets with the LIVE values (ra = the RE'd guest return-address constant, not a magic number),
  `c->r[29] += N` before return. Then the guest-stack bytes byte-match and SBS is 0-diff. This IS the
  port — `game/world/object_table.cpp` is the reference (also beh_pickup_collect_trigger /
  typed_anim_spawn / a06_script_fades). "Diverges at 0x801FE9xx because native doesn't replicate the
  stack frame" is NOT a reason to revert, leave unwired, or add an `isDeadStackScratch`-style exclusion —
  mirror the frame. A dead-scratch exclusion is a last resort ONLY for a slot proven unread AND proven
  impossible to mirror. Run `external/psxport/tools/abi_extract.py <addr> --contract`/`--scaffold` FIRST: it derives frame
  size / spill offsets / ra constants / callee-saved liveness straight from `generated/`
  (`external/psxport/docs/abi-extract.md`). For a NEW port prefer `external/psxport/tools/port_gen.py` (byte-faithful draft) +
  `external/psxport/runtime/recomp/guest_abi.h` + `external/psxport/tools/port_check.py` (`docs/port-framework.md`). See also
  `external/psxport/docs/faithful-execution.md`.

## Render — reimplement, don't transcribe

`pc_render` reads scene data (camera/view, per-object transform, geomblk prims) and draws with float
matrices + a real depth buffer (`gpu_draw_world_quad` / `gpu_vk_draw_*_f`). NO GTE compose, NO `gte_op`
for render, NO byte-packed PSX packet. The engine owns ordering — real depth for 3D, explicit layer/sort
for 2D. `psx_render` is the visual reference / substrate renderer; never the source of draw order, never
a byte-match target. `pc_render` reads other classes' state (fade → `ScreenFade`, camera →
`CutsceneCamera`, …) and **MUST NOT WRITE guest memory** — that is what would break the pc_faithful
byte-compare invariant. Wanting to write guest RAM from render code means the abstraction is wrong.

**No stamping, no tagging** (USER directive, 2026-07-22: NATIVE PRESENTATION). Recovering meaning by ANNOTATING guest packets is transitional debt to be
REMOVED, not extended. Named debt to retire: `withDepthTag` / `gpu_obj_depth_add` / `obj_depth_lookup`
(packet-span → object-depth stamping), `gpu_native_cover_add` / `nativeCoverLookup` (native-cover span
registry), `PktSpanSession` + `ffspan` (pool-span provenance) on the SHIPPING path, and fps60's prim
`matchAndLerp` fingerprinting. The replacement is always the same: RE and port the emitter, then draw
from the object/effect state it owns — a native producer needs no tag, identity is structural. Much of
this is already DEAD (pc_render does not walk the guest OT at all, so tagging meant to rescue OT-walked
prims cannot fire on the field leg): delete dead mechanisms outright, no tombstones. **DIAGNOSTICS ARE
EXEMPT and stay** — `OtAttr`, `PSXPORT_PRIMAT`, `debug objid`/`otattr` read guest state to ANSWER
QUESTIONS, never to produce the picture.

**LERP IS NATIVE TOO** (USER, 2026-07-22), with no exemption for being hard. Interpolate at the ACTOR-TRANSFORM tier: lerp
the per-object state the native producer already owns, then draw, SAME render path for real and
interpolated frames, one frame behind. No guest re-run, no guest writes — an interpolated frame is a
host-side presentation concern. Anchor/stamp special-casing is its own debt, not the replacement.

## Verification

- **Bug-hunt loop + oracle integrity: `docs/bug-hunt-workflow.md` (read first).** Find bugs by oracle
  compare across the PC SKIP × RENDERER matrix; NEVER debug a divergence before the divergent call chain
  is FULLY OWNED end-to-end (grow ownership first). The SBS oracle (core B) must stay pure — only
  PlatformHle + the `gen_func_*` body. Engine/game natives MUST install via `overrides::install(...)`,
  which carries the `gen` body the oracle leg runs and installs the shared oracle-gated thunk; NEVER call
  the raw `shard_set_override`/`ov_<tag>_set_override` for an engine/game native (that reintroduces the
  fake-0-diff-on-core-B bug). A sudden 0-div where SBS used to diverge = suspect the oracle.
- **Job #1 — SBS byte-exact.** `PSXPORT_SBS_MODE=full`. Every diff is fatal. Root-cause + fix.
- **Rendering (once byte-exact) — observable result.** Agent builds, USER eyeballs. Non-visual RAM/state
  probes fine. Don't gate render on reproducing PSX packets — that forces transcription.
- **Content-interface correctness must hold.** Where native and substrate share guest RAM / scratchpad the
  handoff must be exact. Native terrain once clobbered scratchpad 0x1F8001C0 → broke collision
  (later-158). Inspect via `r`/`rw`/`dumpram` (+ `.spad` sidecar; main RAM is BLIND to scratchpad and GTE
  regs).
- **Every A/B leg must PROVE its build succeeded and PROVE which leg it is.** Check the build exit status,
  then gate on an in-band signal distinguishing the legs — a diagnostic-channel count (`ctrl=` lines,
  producer emissions), not the pixels you are measuring; equal pixel counts are evidence ONLY when the
  channel counts differ. Measured, not hypothetical: an A/B that rebuilt for its second leg, failed to
  build, and re-ran the FIRST leg's binary reported a clean "0 pixels changed" for a producer that was
  emitting — twice in one day. Run A/B legs in YOUR OWN CLONE: a temporary-revert A/B in a shared
  checkout leaves the source and `scratch/bin/tomba2_port` disagreeing for the whole run, so every other
  session there measures a binary that does not match what it reads. If someone else's in-flight edit
  broke the tree, do NOT revert their work — compile your own translation units from
  `build/compile_commands.json` and retry the full build later. And land a cross-repo change
  FRAMEWORK-FIRST in ONE commit: an `external/psxport` gitlink bump and the `game/` use of the new
  `GameConfig` field go together, or every other session cannot build (real instance: `game_config.cpp`
  + `bootFmv`).
- **Improve tools when they fall short.** Grep `docs/gfx-debug.md` + `tools/` first; extend, don't
  reinvent; update doc + skill in the same change.

## Build / drive / repo

- **`scratch/bin/tomba2_port` IS THE GAME** — recompiled MAIN.EXE (`generated/shard_*.c`) + native game
  (`game/*`) + PSX platform (the framework's `external/psxport/runtime/recomp/*`). Build = **CMake**; `cmake/tomba2_port.cmake` owns the
  source list (keep in sync when adding files; every `game/` subfolder is on the include path).
  `./run.sh [disc.chd]` extracts MAIN.EXE + builds + runs; rebuild-only is
  `cmake --build build --target tomba2_port` (configure once with `cmake -S . -B build`).
- **Drive/observe:** REPL (`PSXPORT_REPL=1`, stdin) or debug server (`PSXPORT_DEBUG_SERVER=1`,
  `external/psxport/tools/dbgclient.py`). Headless render: `PSXPORT_VK_HEADLESS=1`. Key REPL: `run N`, `newgame`, `skip N`,
  `press/release/tap <btn>`, `r`/`rw`/`w`, `dumpram <path>` (+`.spad`), `shot <path>`, `debug <chans|all>`,
  `stage`/`regs`/`seq`/`quit`. See `docs/driving-the-game.md`.
- **TOOLS ARE PYTHON.** `run.sh` is the ONLY shell script allowed to exist (USER, 2026-08-16: *"only
  run.sh should be a shell script, all other scripts should be python"*) — it is the user's launcher,
  not a tool. Every gate/sweep/builder is a `.py` with `argparse`, a `--help` that states what it
  asserts, and exit codes separating PASS / FAIL / REFUSED (could not assert anything). Add no new
  shell scripts; convert any you touch.
- **Repo layout:** `game/` — PC-native game by SUBSYSTEM FOLDER (`ai/ object/ world/ render/ camera/
  scene/ audio/ input/ player/ ui/ items/ math/ core/`) plus top-level `game_tomba2.cpp` +
  `tomba2_types.h`; put a new native in its subsystem folder, never a grab-bag file. The PSX→PC platform is FRAMEWORK-RESIDENT at `external/psxport/runtime/recomp/` —
  common PSX→PC platform (dispatch/hle/boot/native_boot; PSX-hw natives gpu_native/spu/gte/mdec/cd;
  CD/XA/FMV subsystems; `Game` + `Sbs`). `generated/` — recompiled MAIN.EXE shards (gitignored) = the
  substrate. `vendor/beetle-psx` — committed GPL-2 fork, GTE/MDEC/SPU/CHD hardware backend (not a
  reference emulator; going off it = porting those native, long-term). `tools/` — recompiler + tooling.
  `scratch/` — gitignored artifacts by type (**never `/tmp`**).

## NO HEDGING — ship the code (USER directive, 2026-07-17, hard rule)

When there is code to write, WRITE IT. Do not withhold, defer, gate, or ask permission for work you can
already do. No "want me to do X?" when X is the clear next action (implement the fix, integrate the
producer, publish the verified change) — do X and report it done. No deferring a fix you understand: "I
identified the mechanism but it needs the user to…" is banned when you can implement it now. No "I can't
verify headless" excuses — you CAN drive the game headless and screenshot any aspect or scene (set the
config, run the replay/REPL, `shot`, READ the PNG); if the claim is "I can't see it", the task is to make
yourself see it. Batch means batch: "port a bunch of things" = ALL of them, every producer, not the safe
one plus a list of omissions. Publish verified work without asking. Surface a genuine BLOCKER (a real
design fork only the user can resolve, a destructive irreversible action) — but a blocker is rare, and
"this is subtle / I'm unsure / it's a shared path" is not one.

## Hard rules

- **Single `main` branch.** Verified milestones committed AND pushed to `origin`
  (`github.com/SomeoneIsWorking/Tomba2Engine.git`).
- Never commit CHDs or machine-specific paths. Disc: CLI arg > `PSXPORT_TOMBA2_DISC` (.env) > `*.chd`
  drop-in.
- Beetle changes in the committed fork, NOT out-of-tree `.patch` files.
