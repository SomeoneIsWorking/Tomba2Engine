# Driving the Tomba!2 port — input, automation, reaching a scene

How to get the PORT (`scratch/bin/tomba2_port`) to a target scene and feed it input, headless or live.
This exists because driving the game keeps getting re-figured-out. Pairs with `tomba2-newgame.md`
(title→New Game menu RE), `tomba2-scene-state.md` (state signals), `render-arch.md`, `config.md`.

## ⭐ REACHING REAL FREE-ROAM GAMEPLAY HEADLESS — `PSXPORT_AUTO_SKIP=1` (read this; it keeps getting lost)
**`PSXPORT_AUTO_SKIP=1` now drives all the way into the real, player-CONTROLLABLE free-roam field** —
implemented as a self-contained auto-drive state machine in `runtime/recomp/native_boot.cpp` (later-240).
It: (0) taps **Cross** until task0 enters the GAME stage (`stage=0x8010637C`); (1) waits for the post-NewGame
**intro cutscene** to start (cutscene-active flag `*(0x1F800137)` → 1); (2) pulses **Start** to SKIP the
cutscene (it does NOT end on its own — Start ends it) until the flag clears, then settles ~2s through the
end-fade. Lands controllable (verified: idle frame-to-frame Δ≈0px, holding a direction pans the camera
~70k px, and Start opens the pause menu). Recipe:
```
PSXPORT_AUTO_SKIP=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_REPL=1 \
  ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE   # then REPL: run 400 ; shot/dumpram ; quit
```
The boot log prints `[autoskip] free-roam reached at frame N` when control is handed off (~f216).

**DEAD env vars / CORRECTED earlier claim:** `PSXPORT_AUTO_GAMEPLAY` and the old numeric `PSXPORT_AUTO_SKIP=500`
were referenced only in docs and **read by no code** — a no-input run never mashed anything; it just sat in the
**attract DEMO** (`stage=0x801062E4`, the game playing predetermined input). The attract demo is PSX-rendered
playback, NOT the GAME free-roam field — the native render orchestrator `ov_render_frame` is DORMANT there (and
even in the GAME field; the field renders via the interpreted overlay entity loop — see later-240 in journal).
Do NOT use the attract demo to judge the native render path.
Verify you're in free-roam (not the menu/cutscene) before rendering: object-list head 0x800FB168 != 0 AND the
cutscene flag `*(0x1F800137)` == 0.

## ⭐ THE EFFECT SANDBOX — `tools/sandbox.py`: spawn an effect, move the camera, step time, capture
USER 2026-08-04: *"tbh I wish we had a test app where we could test these things like spawn an effect
etc"*. This is that. It is **not a separate test binary** — a second executable would carry its own
renderer/camera/object graph and drift from the game, i.e. test something the real game never does.
It is a CLIENT that drives the real `scratch/bin/tomba2_port` over the debug server, so it exercises
the code path that ships. With the sandbox not in use, nothing in the game differs.

```
tools/sandbox.py --list                          # the effect recipes + the RE source of each
tools/sandbox.py --selftest                      # prove the parser fires; no game needed
tools/sandbox.py scenarios/banner-camera-pan.txt # launch an instance + run a scenario
tools/sandbox.py --attach 5960 scenarios/x.txt   # drive an instance already up
tools/sandbox.py --sheet scratch/screenshots/n71 # contact-sheet a captured frame series
```

**Spawning goes through the game's own spawner — there is no new engine mechanism.** The debug
server's `call` already invokes a guest function on the live CPU at a frame boundary, so every recipe
is just the game's real spawn entry point with the game's real arguments. Adding an effect to the
registry is RE work (find the spawner), never new code. A recipe whose call returns `v0=0` is a
**FATAL** "SPAWN FAILED", not a skip — otherwise a scenario that spawned nothing would produce a
clean-looking frame series and read as a pass.

Worked example — the item-announcement banner, raised on demand anywhere in the field:
```
$ python3 external/psxport/tools/dbgclient.py --port 5960 "call 80040AA4 38 0"
call 80040AA4(a0=00000038,a1=00000000,...) -> v0=800FB218 v1=00007C7E
```
`FUN_80040AA4` = `CubeTextLedger::spawnPopup(value, variant)`; `value` indexes the string table at
`0x800A33C8` (stride 12) — entry 56 = "A Red Treasure Chest", entry 2 = "Go to the Burning House!",
entry 1 = the game-start banner. `v0` is the node it allocated.

**Scenario files** (`scenarios/`) are one command per line; `#` **only at line start** is a comment;
anything not a runner directive passes to the debug server verbatim, so the whole `help` surface
stays reachable. Runner directives: `spawn <recipe>`, `capture <n> [dir]` (step one LOGIC frame +
shoot, n times), `preseq <n> [dir]`, `sleep <s>`, `echo <text>`.

**Camera motion is the game's own** — `press right` walks Tomba and the follow camera tracks him,
which is exactly the condition the user plays in. `tp <x> <y> <z>` pins the camera and bare `tp`
releases it (see below).

**`capture` is BLIND to fps60 interpolated frames by construction** — it steps whole logic frames.
For a TEMPORAL artefact (vibration, judder, an interp seam) use `preseq`, which dumps PRESENTED
frames and therefore interleaves real and interpolated ones. Pair it with `debug preseqobj`: the emit
path then logs one line per drawn RqItem per present, `key=` being the **node address**, so a single
effect's screen position can be followed frame by frame and differenced. That is how kanban #71 was
measured.

### `preseq` and `tp` now work on the DEBUG SERVER, not just the REPL (2026-08-04)
Both used to exist only in `PSXPORT_REPL`, and this doc says right below that the REPL **blocks the
frame loop** and must not be used for interactive headless work — so the one instrument that can see
interpolated frames was unreachable from any live session. `dbg_server.cpp` now forwards both to the
same functions the REPL calls (`gpu_vk_preseq_arm`, `hooks->replCamTeleport/Off`). No new mechanism,
no env var, no behaviour change.

## ⭐ DETERMINISTIC SCENARIO REPLAYS — `PSXPORT_PAD_REPLAY=replays/<...>/<name>.pad`
For scenarios AUTO-NAV/AUTO-SKIP can't drive (walking into a hut, reaching a specific door, a
visual bug that needs real navigation), use a **recorded pad replay** — a captured button sequence
reproduced bit-for-bit. The categorized library lives in **`replays/`** (see `replays/README.md`
for the full index + how to record one). Replay any scenario headless or under SBS-full:
```
PSXPORT_PAD_REPLAY=replays/scene-transitions/hut-entry-door-freeze.pad   # the key one: surfaces the hut-entry SBS diverge @0x801FE91A
```
Record a new one on a windowed run: `PSXPORT_PAD_RECORD=replays/<cat>/<name>.pad ./run.sh`,
play the scenario, close, add a README entry. **If the user ALREADY has the game open on the bug, do
not make them replay it into a chosen sink** — cut the replay straight out of the live session over
the debug server (`padrec save`, see §Live session below and the `live-session` skill). Replays start from frame 0 (fresh boot) — prefix with
`newgame`/AUTO_SKIP to reach free-roam first if the scenario begins there.

## ⭐ LIVE SESSION — attach to the user's running window, don't relaunch
The user's `./run.sh` window exposes the debug server on **5959** and keeps every finalized pad mask
in memory from frame 0. So a bug they are looking at right now is fully capturable without costing
them the route back to it:
```
python3 external/psxport/tools/dbgclient.py frame                                  # attached?
python3 external/psxport/tools/dbgclient.py padrec                                 # frames captured
python3 external/psxport/tools/dbgclient.py padrec save replays/bugs/<name>.pad    # cut the replay
python3 external/psxport/tools/dbgclient.py padrec save replays/bugs/<name>.pad 9000  # trim the idle tail
```
The optional count keeps the FIRST n frames only; a suffix is never offered (replays are valid only
from boot). `ents`/`node`/`stage`/`shot`/`provat` all work against the live window too.

**Port isolation:** the user's game owns 5959. An instance you launch MUST take another port
(`PSXPORT_DEBUG_SERVER=5960`) or its bind silently fails and your `dbgclient` commands drive the
USER'S game instead of yours. Reach yours with `dbgclient.py --port 5960`.

Fallback for a window launched before `padrec` existed (2026-07-21): a windowed run also writes
`scratch/bin/pad_session.pad`, fflushed every frame, so it is a valid complete-so-far capture at any
instant — just `cp` it. That sink rotates 5 deep on each windowed launch (`.pad` → `.1.pad` → … →
`.5.pad`), so a long capture reappearing as `pad_session.1.pad` means the user relaunched, not that
anything was lost.

## 0. Gotchas that waste time
- **Headless runs auto-SKIP the intro FMVs and fast-forward in-game FMVs** (later-134). A field probe is
  ~1.4s, not ~77s — the intro movie used to be played back in REAL TIME even headless. Just use
  `PSXPORT_VK_HEADLESS=1`; no flag needed. (`PSXPORT_NO_FMV=0` forces FMVs back on if ever required.)
  Standard fast field probe: `PSXPORT_DEBUG=<chan> PSXPORT_GEOMBLK_FRAME=600 PSXPORT_ASPECT=16:9
  PSXPORT_VK_HEADLESS=1 PSXPORT_AUTO_GAMEPLAY=1 PSXPORT_NATIVE_FRAMES=620 PSXPORT_NOAUDIO=1` → field at
  present-frame 328, stable thereafter.
- **Headless is silent automatically** — audio opens only for a real window (`PSXPORT_GPU_WINDOW`); a
  headless / `PSXPORT_VK_HEADLESS` run never touches the sound device. (`PSXPORT_NOAUDIO` still mutes a
  windowed run. WAV capture `PSXPORT_WAV` is independent and works headless.)
- **Headless exits at 120 frames unless you set `PSXPORT_NATIVE_FRAMES=N`.** For a long/interactive run
  set it high (e.g. `=100000`). For a capture, set it just past your last frame.
- **Never `pkill -f tomba2_port` from a shell** whose own command line contains "tomba2_port" — `-f`
  matches the full command line and kills your own shell (exit 144). Use `pkill -x tomba2_port`.
- Backgrounding the port with the agent Bash tool's `&` gets the process group reaped — the live debug
  server then dies. Run it in a persistent session (a real terminal / instance-control) if you need the
  server to stay up across agent turns.

## ⭐ HEADLESS INTERACTIVE — a long-lived instance you can `pause`/`step`/`shot` over the debug server
The trap (re-hit 2026-07-22, kanban #20): `PSXPORT_REPL=1` with no stdin BLOCKS the frame loop waiting to
read, so the debug server times out; and if stdin reaches EOF the REPL quits and the loop exits
("frame loop done" right after "[dbgsrv] listening"). **Do not use the REPL for this at all.** Headless +
`PSXPORT_DEBUG_SERVER` is already the supported interactive shape: `native_boot.cpp` sets `nframes = 0`
(run forever) exactly for that combination, so no `PSXPORT_NATIVE_FRAMES` is needed and the 120-frame
headless cap does not apply.
```
nohup env PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=5960 PSXPORT_AUTO_SKIP=1 \
  ./scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE > scratch/logs/run.log 2>&1 &
# ~20s later it is in free-roam ("[autoskip] free-roam reached at frame N"); then, from any shell:
python3 external/psxport/tools/dbgclient.py --port 5960 frame     # frame=NNNN paused=0
python3 external/psxport/tools/dbgclient.py --port 5960 pause     # freeze (repaints the last real frame)
python3 external/psxport/tools/dbgclient.py --port 5960 vkshot scratch/screenshots/x.png
python3 external/psxport/tools/dbgclient.py --port 5960 step 1    # advance exactly one real frame
python3 external/psxport/tools/dbgclient.py --port 5960 play
```
- **The client is `external/psxport/tools/dbgclient.py`**, not top-level `tools/` (it moved in the repo
  split). `--port` comes BEFORE the command.
- **Use your own port.** 5959 is the user's live window (see §Live session); an agent instance takes
  5960+ or its bind silently fails and the commands drive the USER's game.
- **Two mod configs at once:** point each instance at its own settings file with `PSXPORT_SETTINGS=` (e.g.
  a copy with `fps60=0`) rather than editing `psxport_settings.ini`, which is the user's.
- **Xvfb is NOT a substitute for a window.** `Xvfb` has no DRI3, so Vulkan reports "No DRI3 support
  detected - required for presentation" and no swapchain is ever acquired — a windowed run under it
  cannot verify anything about the presented picture. Verify present-path work headless via `vkshot`
  (which reads the composite target the window samples).

## 1. Pad buttons (active-low 16-bit mask; a pressed button CLEARS its bit; PAD_NONE = 0xFFFF)
| button | bit | active-low hold mask | | button | bit | active-low |
|---|---|---|---|---|---|---|
| Start | 0x0008 | `FFF7` | | Cross/X | 0x4000 | `BFFF` |
| Select| 0x0001 | `FFFE` | | Circle/O| 0x2000 | `DFFF` |
| Up    | 0x0010 | `FFEF` | | Triangle| 0x1000 | `EFFF` |
| Right | 0x0020 | `FFDF` | | Square  | 0x8000 | `7FFF` |
| Down  | 0x0040 | `FFBF` | | | | |
| Left  | 0x0080 | `FF7F` | | | | |
The game reads input as EDGES (`current & ~prev`, FUN_800788ac), so a menu advances once per press; a
held direction is what gameplay reads for movement.

## 2. Scripted (deterministic, headless) — env flags
- **`PSXPORT_AUTO_SKIP=1`** — THE way to reach real free-roam gameplay headless. Drives title → NewGame
  (Cross) → GAME stage → SKIPS the intro cutscene (Start, keyed on the cutscene flag `*(0x1F800137)`) → hands
  off in the controllable field. See the ⭐ callout at the top for the full recipe + verification.
- ~~`PSXPORT_AUTO_NEWGAME`~~ / ~~`PSXPORT_AUTO_GAMEPLAY`~~ — **DEAD** (read by no code; referenced only in
  stale docs). A no-input run sits in the attract DEMO, never the GAME field. Use `PSXPORT_AUTO_SKIP=1`.
- **`PSXPORT_FORCE_BUTTONS=<hex>`** — pulse a mask (8 frames on / 24 off, = edges) from frame 0.
- **`PSXPORT_FORCE_HOLD=<hex>` + `PSXPORT_FORCE_HOLD_AT=N`** — HOLD a mask continuously from frame N
  (overrides the pulse; use for movement, or a single edge by also setting STOP_AT a few frames later).
- **`PSXPORT_FORCE_STOP_AT=N`** — release ALL forced input at frame N (go hands-off).
  Example — press Start once at ~f760: `PSXPORT_FORCE_HOLD=FFF7 FORCE_HOLD_AT=760 FORCE_STOP_AT=768`.
  NOTE the FORCE frame counter is the pad-service frame `s_fc`, which may differ slightly from the present
  frame used by `PSXPORT_VK_SHOTSEQ`.
- **`PSXPORT_SBS_AUTONAV=combat`** (SBS-only, `runtime/recomp/sbs.cpp`) — after the standard
  `SBS_AUTONAV` Nav machine reaches player control, holds Right and fires a jump edge every 60
  frames from frame 300 onward: walks Tomba out of the seaside spawn past the first
  `ActorZonedAttacker` encounter into the melee-cluster zone (`ActorMeleeEngage::doIt`/
  `MeleeProximity::isAtApproachAnchor`, 0x80112188/0x8001F9DC) — the combat-cluster coverage the
  standard `SBS_AUTONAV=1` gate never reaches (docs/findings/ai.md). Pair with `PSXPORT_DEBUG=
  combatnav` to watch it navigate (prints Tomba's world position + pad-drive state every 100
  frames) and `PSXPORT_DEBUG=ovhit` to confirm the addresses fire.

## 3. Live / interactive — the debug server (drive while it runs)
Launch with `PSXPORT_DEBUG_SERVER=1` (port 5959) **and a high `PSXPORT_NATIVE_FRAMES`**. Drive with
`tools/dbgclient.py <cmd>` (or no arg = REPL):
- `tap <btn> [frames]`, `press <btn>`, `release <btn>` — btn = `start x o triangle square up down left right select`.
- `stage`, `scene` (on-demand classified display list), `frame`, `r <addr> [n]` / `rw <addr> [n]` (read mem).
- `shot [path]` writes **PNG by default** (any path not ending in `.ppm` → PNG via SDL3_image; pass
  `foo.png` or just `foo`). No PPM→PNG convert step — the Read tool renders the PNG directly.
- `vkshot [path]` (headless VK readback → PPM), `shot [path]` (VK-aware: captures the PRESENTED picture,
  falls back to SW VRAM when VK is off), `gputrace [path]` (arm a gpu_differ capture).
- `preseq <N> [dir]` — dump the next N PRESENTED frames (default `scratch/screenshots/preseq`) as
  `p%04d.ppm`. Fires once per present PASS, so with 60fps on the sequence interleaves REAL and
  INTERPOLATED frames — the only headless way to see temporal artifacts (interp flicker/judder).
  Analyze with `python3 tools/preseq_flicker.py <dir>`: per-band frame-to-frame displacement via
  profile cross-correlation; alternating-sign motion (the 30Hz oscillation class) is flagged FLICKER.
  - PER-OBJECT variant: run `debug preseqobj` alongside `preseq <N>` and the emit path ALSO logs one
    stderr line per drawn RqItem (`[preseqobj] p<idx> key=… layer=… x=… y=… scene=…`), keyed to each
    present. Feed the captured stderr to `python3 tools/preseqobj_check.py <log>` — the per-object
    acceptance gate that flags any billboard/2D object that OSCILLATES or STALL-STEPS across the presents
    (the fps60 per-object verification instrument; see docs/config.md `preseqobj` + docs/findings/render.md).
    Typical field run: `debug preseqobj` → `preseq 24 scratch/screenshots/preseq_field` → `run 20` → `quit`;
    the fps60 setting comes from `PSXPORT_SETTINGS=<ini with fps60=1>`.
- `pause` / `play` / `step`.

### RE commands (later-134) — inspect/poke/call live, no recompile-a-probe loop
- `w8 A V` / `w16 A V` / `w32 A V` — poke a byte/half/word into guest RAM (hex addr + value).
- `call A [a0 a1 a2 a3]` — run the guest function at A on the live CPU context (rec_dispatch), report
  `v0`/`v1`. SIDE EFFECTS ARE REAL (runs at the frame boundary). E.g. `call 80051c8c <node>` builds an
  object's transform; `call 80051b04 <cmd> <group> <sub>` exercises the geomblk leaf.
- `ents` — walk BOTH entity lists (heads 0x800fb168 / 0x800f2624): per node `addr type pos handler
  rflag cmds geomblk`. The fastest way to see what's spawned + each object's render-command count.
- `node A` — decode one entity node (type/state/rflag/handler/pos/rot/model/cmd-list at node+0xc0[]).
- `geomblk G S` — model-table lookup `T=*(0x800ECF58+G*4); geomblk=T+*(T+S*4+4)` (the data-driven
  geometry resolver, RE later-132).
- Headless-present NOTE: `vkshot` crops to the 4:3 display region; widescreen present width (428) is a
  separate TODO, so the wide margin is in the OT but won't SHOW in a shot yet — verify it via `rcmd`.

## 4. Scene-state signals (RE — to know WHERE you are without screenshots)
- `*(u8)0x800BE258` — **0** = StrPlayer/overlay (logos/title/Loading); **2** (sticky) = 3D engine live
  (gameplay OR attract demo).
- `*(u8)0x1F800137` — **!=0** = real play; **==0** = attract-demo driver running.
- stage pointer `*(u32)0x801fe00c`; GAME stage = `0x8010637C`.

## 5. Reaching free-roam playable gameplay headless — SOLVED (mash Start via AUTO_SKIP; see ⭐ callout)
`AUTO_GAMEPLAY` lands (post-intro, stage GAME) on the seaside→green-field with quest banner "Go to the
Burning House" and an **auto-appearing menu "Options / Load data / Quit game"** (feather cursor on
"Options"). This menu appears WITHOUT input (~f700). The earlier note that it "does NOT respond to forced
Start/Circle/Down" is **partly falsified (later-112): forced Cross (0x4000) at this menu DOES select the
cursor item** — with the cursor on "Options" it enters the Options submenu (page 3 → `FUN_8007b45c`;
verified via the `ov_options_menu` override hit, `PSXPORT_DEBUG=ui`). So this IS the live in-game pause
menu, not a stuck/save-prompt state. Its state machine is RE'd in `docs/engine_re.md` "In-game pause /
Options menu" (page byte `task+0x6B`, dispatch table `0x801062EC`). **Still open:** driving from this menu
into controllable *free-roam* (cleanly closing it back to play).

**UPDATE (later-173): free-roam IS reachable headless — the first half of §5 is SOLVED.** The "auto-appearing
menu" was an artifact of OVER-pulsing Start: `AUTO_GAMEPLAY` releases input at f328 while the post-arrival
fisherman DIALOG cutscene is still up, so Tomba is never controllable. The fix is to keep pulsing Start
through the dialog with **`PSXPORT_AUTO_SKIP=500`**, THEN hold a direction with `PSXPORT_AUTO_WALK`:
```
PSXPORT_DEBUG=state PSXPORT_VK_HEADLESS=1 PSXPORT_AUTO_GAMEPLAY=1 PSXPORT_AUTO_SKIP=500 \
  PSXPORT_AUTO_WALK=r PSXPORT_NATIVE_FRAMES=1600 PSXPORT_NOAUDIO=1 scratch/bin/tomba2_port
```
Tomba then WALKS — camera pos `_DAT_1f8000d2/d6/da` pans (holding right ~3270→5330, left ~4012→3991), and a
screenshot (`PSXPORT_VK_SHOTSEQ`) confirms the green village field is reached with Tomba present. This is a
**deterministic free-roam MOTION scene** (useful for verifying camera-follow / animation systems, which the
idle field — static, A==B — cannot exercise).
- **Knowing the state RELIABLY — `PSXPORT_DEBUG=state`.** Dumps all 3 cooperative-task slots (state@+0x00,
  entry@+0x0c at `0x801fe000 + i*0x70`) on change. A pause/in-game menu is a SEPARATE task — if one ever
  spawns it shows as a slot going alive with a new entry. **CORRECTION of an earlier (later-173) claim made
  from a BROKEN probe:** there is **NO pause menu** in these runs — across AUTO_GAMEPLAY / +AUTO_SKIP /
  +AUTO_JUMP, 0 menu tasks ever spawn and no new task appears after f178 (s0 = the GAME stage 0x8010637C runs
  throughout). The old `nav` probe read `task+0x6B` off the WRONG task (the scheduler's current-task pointer,
  not the menu task), so its "pausePage" / "Cross opens a menu" readings were garbage. **Cross is just JUMP.**
- **Movement geometry (seaside start area):** purely HORIZONTAL — Up/Down move nothing (cam Z stays 2352);
  Left/Right hit hard walls at cam-X ≈ 3991 / 5330. The hut has a visible door but a barrel blocks Tomba at
  the right wall BEFORE the door, so "walk right then Up" does not enter it.
- **`PSXPORT_AUTO_WALK` is a small input SCRIPT** (counted from max(field-reached, AUTO_SKIP)): a single token
  `r`/`l`/`u`/`d`/`x`(Cross/jump)/`o`(Circle)/`t`(Triangle)/`s`(Square) HOLDS that button forever; tokens
  combine (`rx` = right+jump); a comma phase-list `r:250,u:300,rx:120` holds each phase N frames in order then
  releases. Use it to drive toward an exit while `PSXPORT_DEBUG=state` watches for `sm[0x4a]==2`.
- **STILL OPEN — reaching an AREA TRANSITION (`sm[0x4a]==2`).** Walking into either wall does NOT transition
  (`sm[0x4a]` stays 1; `PSXPORT_DEBUG=stage` logs `sm[0x4a]`/`sm[0x4c]`). The seaside area's exit is not a
  plain walk/jump into an edge. Confirmed `ov_game_s4c` (0x80106478, the sm[0x4c] area machine) is NEVER
  entered on the field NOR during the boot area-load (`PSXPORT_DEBUG=stage` ENTER log, 0 hits) — sm[0x4c]
  there is driven by the steady handler 0x801088d8, not 0x80106478. So verifying `ov_game_s4c` needs either
  visual steering of Tomba to the exit, or RE of the exit trigger inside 0x801088d8 (GAME.BIN overlay).

## ⭐ DRIVING BY POSITION FEEDBACK — figure out ANY route (2026-07-15)
The replays are working input sequences; `tools/pad_decode.py <file.pad>` decodes one into its button
timeline (e.g. hut-entry = boot cross/start taps, then **right to the door, then hold up to enter**),
and `--keys` emits an SBS_KEYS string. `--keys-from "220-254:right,..." --out r.pad` builds a .pad from
a spec. So you can READ a route and AUTHOR your own — no live capture needed.

**Movement calibration** (post-control, standalone REPL `press/release` or SBS_KEYS):
- Tomba world pos: **X = *(s16)0x800E7EAE**, **Z = *(s16)0x800E7EB6** (actor base G_ADDR 0x800E7E80 +0x2E/+0x36).
- **left/right = ∓/±X**, ~0x16 (22) units/frame. **up/down = ±Z but ONLY near a door/path** (in the open
  field up/down is walled — Z stays put); near a door, up walks INTO the screen toward it.
- Stage-machine base **sm = *(0x1f800138) = 0x801FE000**; **sm[0x4c] (0x801FE04C) = sub-scene** (==3 = HUT
  interior, per docs/findings/scene.md #49); area-load flag 0x1f80019b (0=loading).

**Worked route — drive into the hut from free-roam (own scripted input, verified):**
1. `PSXPORT_AUTO_SKIP=1` (standalone) or `PSXPORT_SBS_AUTONAV=1` (SBS) → player control (~f216 / ~f246).
2. Walk **right ~34 frames** → the hut door at X≈0x1240 (from start X≈0x0F64).
3. **Hold up ~180 frames** → Z climbs 0x0F80→0x1200, **sm[0x4c] latches 3 (hut entered)**.
   REPL proof: sm[0x4c]=0x03 after right34/up180. SBS recipe (both cores, 0-diff f9480):
     PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_SBS_KEYS="250-284:right,285-470:up"
(The `hut-entry-door-freeze.pad` holds up 300f and hits the freeze bug; the clean `hut-entry-alt.pad`
taps up briefly then walks right through — decode both with pad_decode to compare.) To reach OTHER
areas: same method — walk to the exit, probe X/Z to find the trigger zone, script it.


## `tools/whatisit.py` — what in the GAME is this guest address?

Answers the question the USER asked bluntly and that this project had no cheap answer for: you can
port a function byte-exactly, gate it, and still not know what object it belongs to. Every one of the
46 live field handlers carries a STRUCTURAL name (substate_edge_orchestrator, scatter_record_dither)
because they were all named from the disassembly, never from the screen.

    tools/whatisit.py 0x80118B10          # which live node runs or reaches this?
    tools/whatisit.py --all               # every live node, nearest the player first
    tools/whatisit.py 0x8012D27C --frame 1400 --replay replays/bugs/seesaw-weight.pad

It drives the replay headless to a frame, walks BOTH entity lists, captures a screenshot at the SAME
instant, and prints each node's world position, distance from the player, and native owner (resolved
through codemap, not a grep). Then you look at the picture and match by position.

Two design points worth keeping:
- **A leaf address will not appear as any node's handler**, because only top-level per-object
  behaviours do. The tool SAYS SO, gives the denominator ("examined 150 nodes across both entity
  lists"), and falls back to the nodes nearest the player so you can find your CALLER. It never prints
  an empty table and lets that read as "nothing found".
- **It does not name anything.** Positions and a picture only. A position is a fact; a NAME is a claim
  needing a source (USER or guest data) — the same rule docs/areas.md applies to areas. Do not let a
  plausible screenshot become an identification.

Y grows DOWNWARD in this game, so a more negative Y is HIGHER — the tool prints that reminder because
reading the stack upside down is the easy mistake.
