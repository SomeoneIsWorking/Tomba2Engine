---
name: live-session
description: Attach to the USER'S already-running Tomba!2 window via the debug server — probe live state (frame/stage/ents/node/shot), and CUT A REPLAY out of the running session with `padrec save`. Use whenever the user says the game is open / "capture from the live game" / reports a bug they are looking at right now, or whenever you were about to launch your own run to reproduce something the user already has on screen. Also covers port isolation and not clobbering their capture.
---

# Capturing from the user's LIVE game

The user plays a **windowed** run (`./run.sh`). It already exposes everything you need. **Do not
relaunch the game to reproduce what the user is already looking at** — a relaunch costs them the
whole route back to the bug, and your own boot can die before it reaches anything (the boot FMV
pacer trips the 3s frame watchdog under some launches).

## Attach

```
python3 external/psxport/tools/dbgclient.py frame     # frame=N paused=0  -> you are attached
python3 external/psxport/tools/dbgclient.py stage     # scene/stage latches
python3 external/psxport/tools/dbgclient.py ents      # entity list: addr type pos handler rflag cmds geomblk
python3 external/psxport/tools/dbgclient.py node <A>  # decode one entity (state, pos/rot, cmd list)
python3 external/psxport/tools/dbgclient.py shot scratch/screenshots/live.ppm
```

`shot` writes a **PPM**; convert and actually LOOK at it (`Read` the PNG) before reasoning about
what scene the user is in:
```
python3 -c "from PIL import Image; Image.open('scratch/screenshots/live.ppm').save('scratch/screenshots/live.png')"
```

`pause` / `step N` / `play` freeze the live game — fine for a few frames of state-watching, but you
are freezing the user's screen, so resume.

**Check `paused` before concluding anything about a live session.** The `P` key toggles the pause
(`.` single-steps), so a user can freeze the game without meaning to; the tell is `frame` standing
still across repeated calls. A frozen game cannot perform the repro you are waiting for.

## Cut a replay out of the live session — `padrec`

Every finalized pad mask is kept **in memory from frame 0**, unconditionally (`Pad::mRecLog`), so a
session already in progress can be captured on demand:

```
python3 external/psxport/tools/dbgclient.py padrec                              # N frames captured
python3 external/psxport/tools/dbgclient.py padrec save replays/bugs/<name>.pad # cut it
python3 external/psxport/tools/dbgclient.py padrec save replays/bugs/<name>.pad 9000   # first 9000 only
```

The optional frame count trims the **tail** (drop the idle after the repro). A suffix is never
offered — replays are only valid from boot, so the file always starts at frame 0. Then add a
`replays/README.md` index entry in the same commit, and replay it headless per that README.

**`padrec` landed 2026-07-21.** A game the user launched before that build does not have it — check
`help` output. For such a session, fall back to the file sink below.

## The file sink (fallback, and what NOT to panic about)

A windowed run also records to `scratch/bin/pad_session.pad`, **fflushed every frame**, so that file
is a valid complete-so-far capture at any instant — just `cp` it. On each windowed launch the sink
rotates 5 deep (`pad_session.pad` → `.1.pad` → … → `.5.pad`). Seeing the user's long capture appear
as `pad_session.1.pad` means *they* relaunched, not that anything was lost.

Headless auto-record is OFF by design (it used to silently truncate the user's real captures). An
explicit `PSXPORT_PAD_RECORD=<path>` records anywhere and is never rotated.

## Isolation rules — do not disturb the live run

- **The live game owns debug port 5959.** Any instance YOU launch must take another port
  (`PSXPORT_DEBUG_SERVER=5960`) — otherwise its bind quietly fails and every `dbgclient` command you
  send lands on the *user's* game instead of yours. Pass `--port` to the client to reach yours.
- **Never `PSXPORT_PAD_RECORD=scratch/bin/pad_session.pad`** and never run your own instance windowed
  (agents are headless-only: `PSXPORT_NOWINDOW=1`).
- **Kill only what you launched.** Capture `$!` at launch and `kill -9 "$P"`, or
  `<local-notes>/skills/safe-kill/safekill -p <pid>` after identifying it in `ps -eo pid,etimes,args`.
  Never `pkill -f tomba2_port` (matches your own wrapper shell and kills the command mid-run). Check
  `etimes` before killing: the long-lived process is the user's game.

## Where this is implemented

- `external/psxport/runtime/recomp/pad_input.{h,cpp}` — `mRecLog`, `recordedFrames()`,
  `saveRecording(path, nframes)`, and the record/replay block in `serviceFrame()`.
- `external/psxport/runtime/recomp/dbg_server.cpp` — the `padrec` command + help text.
- `docs/driving-the-game.md` — the wider driving/automation doc (AUTO_SKIP, replays, REPL).
- `replays/README.md` — replay format, index, how to replay under SBS.
