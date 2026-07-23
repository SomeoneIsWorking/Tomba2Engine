# Replay library

Deterministic pad-capture replays — each is a recorded button sequence (2-byte active-low PSX pad
mask per frame, written by `PSXPORT_PAD_RECORD=<path>`) that reproduces a specific game scenario
**headlessly, bit-for-bit**. Use these to reproduce a bug or reach a game state WITHOUT live user
input — critical for scenarios that need real world-navigation (e.g. walking into a hut) that
AUTO-NAV can't drive.

## How to record a new replay

On a **windowed** run (the user's, or your own with a window), input is recorded by default to
`scratch/bin/pad_session.pad`. To capture a specific scenario to the library:

```
PSXPORT_PAD_RECORD=replays/<category>/<name>.pad ./run.sh
# play the scenario with the keyboard/gamepad, then close the game
```

Then add an entry to this README (category / scenario / how-to-replay / what it surfaces) in the
SAME commit. Categories: `boot-smoke/` (boot + menu nav), `scene-transitions/` (entering
huts/areas/doors), `bugs/` (repros for a specific tracked bug).

## How to replay one

```
# standalone (REPL, inspect state the scenario produces):
printf 'newgame\nrun 9999\nquit\n' \
  | PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 \
    PSXPORT_PAD_REPLAY=replays/<category>/<name>.pad ./scratch/bin/tomba2_port

# under SBS-full (surface memory divergences the scenario reaches):
PSXPORT_NOWINDOW=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 \
  PSXPORT_PAD_REPLAY=replays/<category>/<name>.pad ./scratch/bin/tomba2_port

# under SBS-full WITH the debug server (pause at the diverge, then `sbs watch`/`sbs bt`):
PSXPORT_NOWINDOW=1 PSXPORT_DEBUG_SERVER=1 PSXPORT_SBS_MODE=full PSXPORT_SBS_AUTONAV=1 PSXPORT_NOAUDIO=1 \
  PSXPORT_PAD_REPLAY=replays/<category>/<name>.pad ./scratch/bin/tomba2_port &
# then: python3 tools/dbgclient.py sbs status   (connects to 127.0.0.1:5959)
```

`newgame` pulses Start to reach the GAME prologue, then the replay's button sequence drives from
there. Replays ALWAYS start from a fresh boot — `PSXPORT_PAD_REPLAY` overrides input from frame 0.
Once the recorded sequence ends, input falls through to the host (so `run N` after it just idles).

## Index

### scene-transitions/

| file | frames | scenario | surfaces |
|------|--------|----------|----------|
| `hut-entry-door-freeze.pad` | 934 | AUTO-NAV to free-roam, then walk right + up into the fisherman's-hut door | **SBS diverge @0x801FE91A (f389)** + a door-transition freeze (`FUN_80073328 case 3`) reaching both cores; see docs/findings/scene.md "Door/area-transition freeze" + docs/findings/sbs.md "spawn-leaf frame residual". The cleanest repro for the hut-entry diverge the live windowed session hits. **Also the headless way INSIDE the hut interior** (kanban #29): `run 1200` with no `newgame` lands standing in the room with the wall decorations on screen — the pad ends at f934 and the transition has completed by ~f1100; `press left` + `run 80` scrolls the camera to a second interior viewpoint, `press right` walks back out onto the field. |
| `hut-entry-alt.pad` | 535 | alternate, shorter hut-entry capture | same hut-entry transition; a second capture for cross-checking. |

### bugs/

| file | frames | scenario | surfaces |
|------|--------|----------|----------|
| `seesaw-weight.pad` | 16406 | live hand-played session reaching the seaside **water pump**, with the counterweight seesaw beam | kanban #8 — grabbing the seesaw while climbing does not sink it under Tomba's weight. Cut out of the user's LIVE window with `padrec save` (see docs/driving-the-game.md § live session). Replay reaches the pump; padshots at 8000/12000/14000/15500/16350 show the beam static. |
| `dark-screen-repro.pad` | 61030 | long session reaching a dark/wrong-screen state | the screen-fade / scene-darkening bug family (docs/findings/render.md); use under a window to eyeball. |
| `house-on-the-point.pad` | 4134 | live hand-played session entering **"House on the Point"** | the entry CORRUPTS GAME STATE (NOT a freeze/crash — the replay runs clean): music stops, the interior geometry vibrates, and on leaving the house the camera stops following Tomba. Cut out of the user's LIVE window with `padrec save`. Verify by STATE/behaviour comparison (camera-follow latch, BGM sequencer state, per-frame interior vertex jitter), not by crash-checking — a headless run will exit 0 while the state is wrong. |
| `ingame-options-page.pad` | 1158 | AUTO_SKIP to free-roam, Start, Cross — lands on the IN-GAME **Select Options** page at frame **1160** | kanban #38 (the page's missing full-screen backdrop). The whole OPTIONS family is reachable from here: tap Down k times + Cross after f1160 for Messages / Sound / Screen adjust / Controls. Frame 1090 is the START page (#35). Capture a fixed frame with the debug server (pause, ONE `step` to the target, `shot`). |
| `title-options-page.pad` | 1115 | title → New Game menu → **Options** — the FRONT-END options page at frame **1027** | the same five page builders reached from the title (kanban #7). Use it to check the title path has not regressed when the shared OptionsPage producer changes. |
| `walk-dust-puff.pad` | 530 | AUTO_SKIP to free-roam, then walk right / stop / tap left / run-and-reverse — recorded headless with `PSXPORT_PAD_RECORD` | kanban #39. Fires the **FUN_800286CC animated-quad effect** (impact starburst / one-shot puff) at frames **451-468**; `debug fxanim` names the node and its quad count. NOT the FUN_80029F6C movement dust — for that use `seesaw-weight.pad` frames **216-231** (`debug fxdust`, `debug otattr`). |
| `ingame-item-menu.pad` | 1118 | AUTO_SKIP to free-roam, Triangle — the in-game **item/pause menu** at frame **1120** | kanban #21 (the menu's opaque panel). Pixel-exact against psx_render at f1120, so it doubles as the #21 no-regression gate. |

### boot-smoke/

| file | frames | scenario | surfaces |
|------|--------|----------|----------|
| `start-mash-smoke.pad` | 1500 | Start-mash through the intro to free-roam | boot→menu→field smoke check (should reach GAME stage cleanly). |
| `short-session.pad` | 426 | short input session | minimal boot capture. |
| `general-session.pad` | 1200 | general play session | generic smoke. |

## Notes

- **Format:** each `.pad` is a raw stream of little-endian `uint16_t` active-low PSX pad masks, one
  per frame. Decode with: `python3 -c "import struct; [print(hex(struct.unpack('<H',open('X.pad','rb').read()[i:i+2])[0])) for i in range(0,len(open('X.pad','rb').read()),2)]"`.
  Active-low bits (0=pressed): 0=R 1=L 2=U 3=D 4=tri 5=cross 6=o 7=sq 8=L2 9=R2 10=L1 11=R1 … start.
- **Determinism:** replays are bit-exact (validated by the door_freeze finding — reproduces
  frame-for-frame across runs). Safe to gate fixes on.
- **Cross-exec-path replay: UNKNOWN, do not assume either way.** One observation (2026-07-21,
  `bugs/seesaw-weight.pad`) had the pc_faithful and `PSXPORT_GATE=1` legs rendering different areas at
  the same replay frame (`scratch/screenshots/seesaw_pc_vs_gate.png`) — but nothing established that
  the input stream was the cause rather than a real behavioural difference between the paths. An
  earlier revision of this file asserted a desync mechanism as fact; that was unverified and is
  withdrawn. For path comparison prefer SBS (lockstep, one input stream, two cores) until someone
  actually measures this.
- **Originals** also live at `scratch/bin/*.pad` (referenced by older findings); the copies here are
  the canonical, categorized home going forward.

- `bugs/weapon-impact-bucket.pad` — 686 frames, self-contained from boot (do NOT add
  `PSXPORT_AUTO_SKIP` or `newgame`, it desyncs). Tomba walks right along the seaside start route into
  the bucket obstacle and swings (CIRCLE); the swing connects at pad frames 654-660, peak 656. The
  repro for kanban #15 (weapon impact burst missing its mesh half under pc_render):
  `PSXPORT_PAD_REPLAY=replays/bugs/weapon-impact-bucket.pad PSXPORT_PAD_SHOT_AT=654,656,658 ... run 700`
