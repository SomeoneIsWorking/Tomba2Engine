# Graphics debugging — workflow + tools (Tomba!2 port)

_This file IS the canonical workflow. `.claude/skills/gfx-debug/` is a thin in-repo skill that points
here — it deliberately does not restate any of this, so there is nothing to keep in sync._
_Read this FIRST before debugging any rendering issue._

## The methodology (2026-06-20): the engine OWNS its render — verify on the LIVE game

There is **no oracle and no render-diff tool anymore** (the Beetle reference emulator + `gpu_differ` +
all the compare scripts were removed by user directive). The engine OWNS its world, projection, and
render ordering PC-native — so there is nothing PSX to diff against. A "renders wrong" bug is found by
inspecting **the running port**, not by replaying a GP0 stream through a second renderer.

For ANY "something renders wrong" problem (water, fade, corruption, color, geometry, order):

1. **Inspect the LIVE game's render state** — drive the port to the scene via the REPL, then use the
   diagnostic channels (enabled at runtime with the `debug <chan>` REPL/debug-server command — NOT an
   env var) and the on-demand inspection commands:
   - `debug scene` — classified per-frame display list (poly/rect/fill/VRAM-copy/env counts, fade/copy
     details). Live debug server: `scene` (on-demand `gpu_scene_dump`).
   - `debug gte` — GTE ops + lighting/fog/projection control regs.
   - live debug server `provat x y` — which prim (op/clut/texpage/node) last wrote a displayed pixel.
     **Only under psx_render.** It tracks CPU `s_vram`, so under pc_render (the default) every probe
     reads `<never written>` — the native overlay draws the picture and never writes guest VRAM. For
     VK polys use `PSXPORT_PRIMAT="x,y"` (row below), not this.
   - live debug server `vkvram x y w h <path>` — read back an arbitrary VK VRAM region to a PPM (verify
     a texture/CLUT page is actually where the sampler expects it).
   - `debug fade` / `debug semi` — fade brightness / semi-transparent prim blend+bbox details.
2. **Build it and have the USER eyeball.** The agent CANNOT self-verify a render — it builds, captures a
   headless screenshot (`PSXPORT_VK_HEADLESS=1` + the REPL `shot <path>` command, VK-readback PPM), and
   asks the user (who is at a Mac and can eyeball pics / pull a push). NEVER conclude a render is correct
   from a single still you looked at — stills hide bugs (water "looked fine" while the user saw it
   broken). The user is the visual authority.

**The engine owns ordering — never re-sync with the PSX.** If something draws in the wrong order (e.g. a
background painted over the scene), the fix is to give the engine the correct ordering RULE (its own
depth buffer for 3D, its own 2D layer/sort for backgrounds/HUD) — NOT to read the PSX OT / GP0 order /
z-otz. There is no PSX draw order to consult anymore.

## Capturing a screenshot to send the user (headless, no display)
```
printf 'newgame\nskip 200\nrun 60\nshot scratch/screenshots/x.ppm\nquit\n' \
  | PSXPORT_VK_HEADLESS=1 PSXPORT_REPL=1 PSXPORT_NOAUDIO=1 scratch/bin/tomba2_port scratch/bin/tomba2/MAIN.EXE
# convert + send: magick/convert x.ppm x.png, then SendUserFile
```
`newgame` pulses to the GAME prologue; `skip N` advances the intro into the field; `run N` steps N
frames; `shot` reads back the VK image over the display region (works headless). Drive elsewhere with
`press`/`release`/`tap <btn>` + `run`.

## The render toolbox (what's left)

| Tool | What it does | Invoke |
|------|--------------|--------|
| **REPL `shot <path>`** | VK-readback of the presented region to a PPM (works headless). The way to capture a frame for the user. | piped REPL command |
| **`debug <chans>`** | Enable diagnostic channels at runtime (scene/gte/fade/semi/provat/objz/ndepth/…) — replaces the old per-flag env vars. | REPL or debug-server command |
| **`debug fadeshot`** | On every recomp screen-fade call `FUN_8007E9C8(color=a0)` (interp.cpp hook), log `call=N color=0xRRGGBB ra=` and capture `s_tex` to `scratch/screenshots/fade_NNN.ppm`. Deterministically captures a transition's fade frames (frame-indexed shots DRIFT across launches). Headless shows the CORRECT single menu→cutscene fade-in; the corrupt EXTRA menu fade only forms windowed (later-250). | `debug` channel |
| **live debug server** | NON-BLOCKING TCP server (`runtime/recomp/dbg_server.c`) — inspect the port WHILE the user plays live (windowed). `r`/`rw`/`stage`/`scene`/`provat x y`/`vkvram`/`shot`/`debug <chan>`/`frame`. | `PSXPORT_DEBUG_SERVER=1` (port 5959); `external/psxport/tools/dbgclient.py <cmd...>` |
| `PSXPORT_VK_HEADLESS=1` | Offscreen VK (no window/display) — deterministic; the basis of the headless screenshot. | env (run mode, not a behavior gate) |
| **`debug otattr` + REPL `otattr`** | OT/GTE SUBMISSION ATTRIBUTION: traces every guest-submitted packet-pool store and every GTE RTPS/RTPT back to the emitting guest fn (rec_dispatch shadow stack) + world-object node (same `cur_render_node` fallback the native submit path uses). Turn the channel on BEFORE the frame you want to inspect, then run the REPL `otattr` command to re-walk the last OT read-only and print `pool= op= fn= caller= node= beh@node+1C=` per packet, plus a per-(fn,node) GTE call-count histogram. Built for bug #45 (multi-quad batching) + bug #34 (dialog-panel emitter) — see `game/render/ot_attr.h`. Blind to fully-native (non-dispatched) draw paths — see the class comment for the limitation. | `debug otattr` then REPL `otattr` |
| **REPL `otattr watch/who/trace`** | LAST-WRITER PROVENANCE — answers "who WROTE this word" (any address, incl. scratchpad), independent of call-flow, for the staging-buffer case where `otattr`'s call-path attribution only names the batcher. `watch <addr> <len>` registers a region (8 slots, 64 KB total); `who <addr> [len]` dumps the word-granular last-writer, coalescing same-writer runs; `trace <addr>` prints the last writer + a copy-loop heuristic (does the writer's stores fan out across many 4KB pages this frame?) and points at a Ghidra follow-up when the source isn't statically determinable. Same shadow-stack signal as `otattr`, so it's ALSO blind past a plain (non-jalr/rec_dispatch) nested call — verified on bug #45's `0x8003F9A8`: last-writer matches call-path attribution exactly (no hidden RAM-copy hop), so the true emitter leaf there needs a Ghidra decompile of `0x8003F9A8`, not this tool. See `docs/config.md` `otattr` section. | `debug otattr` then REPL `otattr watch/who/trace` |
## THE THREE GROUND TRUTHS — and the one we keep forgetting

When a visual is wrong there are exactly three things that can settle it. Reach for them in this order,
because they get STRONGER down the list, not weaker:

1. **psx_render (the in-process `renderpsx` toggle).** Cheapest, and right most of the time. But it is
   NOT ground truth — it has its own render bugs, and when a fault is shared by both legs the compare
   returns 0 diff and reads as "nothing to fix". Two measured instances: the triangle menu (both legs
   pixel-identical and both wrong) and the health wheel. A clean diff is not a clean bill of health.
   **TWO WAYS THE TOGGLE SILENTLY RETURNS A pc FRAME while you think you have psx** — both have burned a
   session (they manufactured kanban #26): (a) a BARE `renderpsx` used to only PRINT the flag and set
   nothing (fixed 2026-07-23 — it now toggles); (b) the flag is honoured per SCENE ENTRY, so flipping
   it INSIDE an already-loaded area does not repaint — the next shot is bit-identical to the pc frame
   even though the flag reads back 1 (kanban #41, not root-caused). So for any AREA/WARP compare, set
   the reference from a SECOND run with `PSXPORT_RENDER_PSX=1` at BOOT, at the same frame index (the
   pad replay is bit-deterministic) — `tools/warpsweep.sh` does exactly this. Never compare adjacent
   frames, and never trust a mid-scene toggle flip.
2. **A real-game reference capture** (USER can source one; store in `docs/reference/issues/`). The
   arbiter when 1 is compromised. Sparse — you get the screens someone happened to photograph — but
   authoritative for those, and two references of the SAME element over different backgrounds tell you
   far more than one (see the health-wheel entry below).
3. **THE GUEST'S OWN SUBMISSION — the one we keep forgetting, and the strongest.** The port OWNS the
   emitters, so what a primitive is SUPPOSED to be is a fact in the data, not a matter of opinion:
   the semi bit and blend mode the guest sets, the CLUT and texpage it selects, the sort key it
   computes, whether an opaque slice is submitted at all. No picture required, and it works precisely
   when both renderers are wrong — which is when 1 fails and 2 may not exist for that screen.
   - "Is the health wheel meant to be semi-transparent?" → read the semi bit on its prims.
   - "Is the menu's opaque fill missing or mis-drawn?" → is an opaque quad SUBMITTED for it at all.
   - "Which face should win?" → the sort key the guest computes (this is how kanban #11 was settled).
   Tools: `PSXPORT_PRIMAT="x,y,frame"` (every prim covering a pixel, with `raw/mode/tp/clut/uv`),
   `debug otattr` + REPL `otattr` (which fn+node emitted a packet), `tools/find_refs.py` (who reads or
   writes a guest address). Reading guest state to ANSWER A QUESTION is always allowed; the ban is on
   the shipping path depending on it.

An image tells you the renderers disagree with reality. The guest data tells you WHICH one is wrong and
WHY — so when a reference is unavailable, that is the answer, not a dead end.

## REAL-GAME REFERENCE CAPTURES — the arbiter when the oracle is also wrong

The default render check is pc_render vs psx_render in ONE process (`renderpsx` REPL toggle, default
leg). That assumes the oracle is right. **It is not always right** — psx_render has its own render
bugs — and when a fault reproduces on BOTH legs the comparison returns 0 diff and reads as "nothing
to fix". That is a false negative, and it is expensive: it retires a real bug as a non-issue.

So when a symptom might be shared by both legs, get a capture of the REAL game (the user can source
one) and treat THAT as ground truth. Store it in `docs/reference/issues/` and cite it on the card.

Two things this bought on kanban #22 (health wheel "too transparent") that no amount of engine
inspection would have:
- **Two references beat one.** The same wheel over bright sky and over dark ground showed the
  background genuinely modulating it — so the wheel IS semi-transparent in the real game and "make
  it opaque" would have been the wrong fix. The direction (darker over dark, brighter over bright)
  is the signature of averaging `0.5B+0.5F` rather than additive, which narrowed the mechanism to a
  missing OPAQUE BACKING rather than a wrong blend equation.
- **A reference scopes the card honestly.** The #21 triangle-menu reference showed the target is not
  just a missing background but a whole stack — opaque fill, border, tabs, legend row, icon tiles,
  divider, scroll arrow, help panel with portrait. "Menu is transparent" alone invites a one-quad
  fix and a report that closes the card with half its layers still absent.

**Ask for a reference** whenever a visual is wrong in a way the oracle may share, or whenever the
correct appearance is not obvious from our own output. Cheap to get, and it is the only ground truth
for this class.

| **`PSXPORT_PRIMAT="x,y"`** | At a DISPLAY pixel, log EVERY prim covering it (point-in-triangle) with is3d/bg/billboard/per-vertex dep/op/tp/clut/col/bbox — from BOTH the gp0 OT-walk (`gpu_native.cpp` gp0_exec) AND the queue/world path (`gpu_emit_rq_item`, tagged `[primat-rq]`). USE THIS, not `provat`, for VK polys: provat tracks CPU `s_vram` and is BLIND to the VK-teed polygons. Tells you what actually contests a pixel and at what depth. The `[primat-rq]` line also carries `seq` (submission order — on PSX the HIGHER seq paints last and wins) and the full texture identity `raw/mode/tp/clut/uv[4]/xy[4]`, so "is this face losing a depth contest or sampling the wrong page?" is answerable from one line. Two prims printing the SAME `xy[4]` set are one surface submitted twice — a painter-order pair (see the barrel entry in `docs/findings/render.md`). Takes an optional third field, `"x,y,frame"`: without it the 6000-line cap is spent in the first few hundred frames and never reaches a late repro. | env or `debug`-channel cfg |
| **`PSXPORT_PAINTFG=1`** | Force every gp0 2D-FG (HUD-band) poly to solid magenta — instantly shows how much of the frame is still flat 2D-band (un-owned) vs real-depth world geometry. (later-229: revealed the seaside field is ~99% 2D-band.) | env / cfg |
| **`PSXPORT_PAINTER=1`** | Force PURE PSX OT painter order for EVERY prim (gp0 tee: is3d=0, no bg split) — composites exactly as the PSX ordering table would. Render the SAME scene with `=1` and `=0` and pixel-diff: differing pixels = where native per-pixel depth changes the picture (the object-occlusion bug — terrain/atlas not obeying world-depth). If `=1` makes the bug disappear, native-depth ordering is the cause. Headless start-field diff was only 39px (that view is ~99% 2D-band already, so native≈painter); run it WINDOWED where the bug shows. | env / cfg (`gpu_native.cpp` gp0_exec) |
| `tools/render_cmp.py` | Native-vs-PSX render compare at 1x/4:3/30fps → `scratch/screenshots/cmp_triptych.png` + diff %. A DIAGNOSTIC GATE (real-depth ownership is the target, NOT matching PSX). `g_render_psx`/`PSXPORT_RENDER_PSX` renders the field via the PSX recomp path for the compare. | CLI |
| `tools/disas.py <addr> [--ram dump]` | MAIN.EXE / overlay disassembly with resolved load/store addrs — read the recomp reference body before reimplementing an engine fn. | CLI |

## Visual settings (widescreen / hi-res / SSAO / lighting / 60fps)
These are NOT env vars — they default OFF and are toggled live in the **F1 imgui overlay**, persisted to
`psxport_settings.ini` (gitignored), restored next launch (`runtime/recomp/mods.c`, `mods.h`). To
reproduce a user's look, set the relevant keys in `psxport_settings.ini` before launching. NB SSAO/light
currently have a known bug hiding some transparent surfaces (puddles/water) — see the journal.

## Honest limits
- **You cannot self-verify a render.** The user is the visual authority — build, send a pic, ask.
- Headless `shot` captures the display region (`s_disp_*`); for a widescreen run the wider FB content is
  rendered but the shot crops to the display width — note this when capturing widescreen.
