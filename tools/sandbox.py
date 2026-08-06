#!/usr/bin/env python3
"""sandbox.py — spawn an effect in isolation, drive the camera at it, step time, capture a series.

WHY THIS EXISTS (USER, 2026-08-04): "tbh I wish we had a test app where we could test these things
like spawn an effect etc". A render bug that only shows up while the camera MOVES cannot be caught by
a screenshot or by a single-frame compare; it needs the same effect raised repeatably, the same
camera path driven twice, and a FRAME SERIES to look at.

WHY IT IS NOT A SEPARATE TEST BINARY. The whole value of this tool is that it exercises the code path
that SHIPS. A second executable would have its own renderer, its own object graph and its own camera,
and would drift from the game within a week — it would test something the real game never does, which
is worse than no test at all. So this is a CLIENT: it drives the real `scratch/bin/tomba2_port` over
the debug server that already exists (external/psxport/runtime/recomp/dbg_server.cpp). Nothing here
changes what the game does; with the sandbox not in use, not one byte of the shipping path differs.

WHY SPAWNING IS `call`, NOT A NEW MECHANISM. The debug server can already invoke a guest function on
the live CPU at a frame boundary. Every recipe below is therefore the GAME'S OWN spawn entry point
with the game's own arguments — the same function the game calls when it raises the effect itself. No
fabricated object graph, no hand-built node: if the recipe is right the game does the spawning, and if
it is wrong the game refuses and we see that (a recipe whose call returns 0 is reported as FAILED, not
skipped). Adding an effect to this tool is RE work — find the game's spawner — never new engine code.

USAGE
  tools/sandbox.py --list                             # the recipe registry, with its RE sources
  tools/sandbox.py --selftest                         # prove the scenario parser fires (no game)
  tools/sandbox.py scenarios/banner-pan.txt           # launch an instance, run a scenario
  tools/sandbox.py --attach 5960 scenarios/foo.txt    # drive an instance that is already up
  tools/sandbox.py --sheet scratch/screenshots/foo    # contact-sheet a directory of frames

SCENARIO FILE — one command per line, `#` comments, blank lines ignored. Anything not listed below is
passed to the debug server VERBATIM, so the full `help` surface (r/rw/w32/ents/node/provat/press/
release/tap/hold/pause/play/step/shot/debug/stage/scene/dumpram/tp/preseq) is available unchanged.
Runner-side directives:
  spawn <recipe>          raise a registered effect through the game's own spawner
  capture <n> [dir]       n times: step ONE logic frame, screenshot -> dir/f%03d.png
  preseq <n> [dir]        arm a PRESENTED-frame dump: with fps60 this interleaves REAL and
                          INTERPOLATED frames, and is the only headless view of a temporal artefact.
                          `capture` steps whole logic frames and is BLIND to interpolated frames.
  sleep <seconds>         runner-side wait (let the game run free)
  echo <text>             annotate the transcript
"""
import argparse, os, socket, subprocess, sys, time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ─────────────────────────────────────────────────────────────────────────────────────────────────
# THE RECIPE REGISTRY — named effect -> the GAME's own spawn entry point.
#
# Every entry must cite where its address and arguments were RE'd from. An entry without a source is
# a guess, and a guess that happens to draw something is worse than no entry, because it looks like a
# test of the real path. `ret0_is_failure` says whether v0==0 means the game refused the spawn.
# ─────────────────────────────────────────────────────────────────────────────────────────────────
RECIPES = {
    "banner:red-chest": dict(
        addr=0x80040AA4, args=(56, 0), ret0_is_failure=True,
        what="item-announcement banner, 'A Red Treasure Chest' (one rounded tile per glyph)",
        src="CubeTextLedger::spawnPopup, game/object/cube_text_ledger.h — FUN_80040AA4(value,variant): "
            "allocates a node, sets node[0x1C]=0x8003AD48 (beh_cube_text_spawn), node[0x60]=value. "
            "value = index into the string table at 0x800A33C8 stride 12; entry 56 is 'A Red Treasure "
            "Chest' (kanban #64, walked end to end). Returns the node ptr, 0 on freelist exhaustion."),
    "banner:burning-house": dict(
        addr=0x80040AA4, args=(2, 0), ret0_is_failure=True,
        what="quest banner, 'Go to the Burning House!' — kanban #16's string, same emitter",
        src="same spawner; entry 2 of the 0x800A33C8 table (kanban #64/#16)."),
    "banner:find-tabby": dict(
        addr=0x80040AA4, args=(1, 0), ret0_is_failure=True,
        what="quest banner, 'Find Tabby!' — entry 1, the one shown at game start",
        src="same spawner; entry 1 of the 0x800A33C8 table (kanban #64)."),
    "banner:clear": dict(
        addr=0x80040AA4, args=(0, 2), ret0_is_failure=True,
        what="the 'Clear' variant banner (node[3]==2 branch of beh_cube_text_spawn)",
        src="same spawner with variant=2; beh_cube_text_spawn STATE 0 takes the "
            "strcpy(\"Clear\")+strcat path instead of the string table (game/ai/beh_cube_text_spawn.cpp)."),
}


class Dbg:
    """One persistent connection to the port's debug server. Every reply ends with ---END---."""

    def __init__(self, port):
        self.port = port
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=30)

    def __call__(self, line):
        self.sock.sendall((line + "\n").encode())
        buf = b""
        while b"---END---\n" not in buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise RuntimeError(f"debug server on {self.port} closed mid-command: {line!r}")
            buf += chunk
        return buf.split(b"---END---\n")[0].decode(errors="replace")


def launch(port, extra_env=None, timeout=90):
    """Start our OWN headless instance. Never 5959 — that is the user's live window, and a failed
    bind is SILENT, so a clash would send every command to the user's game instead of ours."""
    if port == 5959:
        sys.exit("sandbox: refusing port 5959 — that is the user's live window (bind failure is silent)")
    os.makedirs(f"{REPO}/scratch/logs", exist_ok=True)
    log = f"{REPO}/scratch/logs/sandbox_{port}.log"
    env = dict(os.environ,
               # PSXPORT_NOPACE: drive the game as fast as the host can. Headless is PACED like a
               # windowed run now (they are one program), so "fast" has to be ASKED for.
               PSXPORT_VK_HEADLESS="1", PSXPORT_NOAUDIO="1", PSXPORT_NOPACE="1",
               PSXPORT_DEBUG_SERVER=str(port), PSXPORT_AUTO_SKIP="1")
    env.update(extra_env or {})
    exe = f"{REPO}/scratch/bin/tomba2_port"
    if not os.path.exists(exe):
        sys.exit(f"sandbox: {exe} does not exist — build first (cmake --build build --target tomba2_port -j6)")
    with open(log, "wb") as fp:
        proc = subprocess.Popen([exe, f"{REPO}/scratch/bin/tomba2/MAIN.EXE"],
                                stdout=fp, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL,
                                cwd=REPO, env=env, start_new_session=True)
    print(f"[sandbox] launched pid={proc.pid} port={port} log={log}")
    # Wait for AUTO_SKIP to hand off in free-roam. Report the WAIT, and fail loudly rather than
    # letting a scenario run against the title screen and read as a result.
    deadline = time.time() + timeout
    while time.time() < deadline:
        if proc.poll() is not None:
            sys.exit(f"sandbox: instance exited early (code {proc.returncode}); see {log}")
        try:
            if "free-roam reached" in open(log, errors="replace").read():
                print(f"[sandbox] free-roam reached after {time.time() - deadline + timeout:.0f}s")
                return proc
        except FileNotFoundError:
            pass
        time.sleep(1)
    proc.kill()
    sys.exit(f"sandbox: instance never reached free-roam in {timeout}s; see {log}")


def do_spawn(dbg, name, out):
    r = RECIPES.get(name)
    if not r:
        raise SystemExit(f"sandbox: no recipe named {name!r}. Known: {', '.join(sorted(RECIPES))}")
    args = " ".join(f"{a:X}" for a in r["args"])
    reply = dbg(f"call {r['addr']:X} {args}").strip()
    out(reply)
    # A spawner that returns 0 REFUSED the spawn. Saying nothing here would let a scenario that
    # spawned nothing produce a clean-looking frame series — the exact "diagnostic that can print
    # nothing" failure. So it is fatal and it says why.
    if r["ret0_is_failure"]:
        v0 = reply.split("v0=")[-1].split()[0] if "v0=" in reply else "?"
        if v0 in ("00000000", "0", "?"):
            raise SystemExit(f"sandbox: SPAWN FAILED for {name!r} — {r['addr']:#X} returned v0={v0}. "
                             f"The game refused it (wrong stage, freelist exhausted, or the recipe is "
                             f"wrong). NOT continuing; a frame series from here would show nothing and "
                             f"read as a passing test.")
        out(f"[sandbox] spawned {name} -> node {v0}  ({r['what']})")


def do_capture(dbg, n, directory, out):
    """Step ONE LOGIC FRAME at a time and shoot. Deterministic and repeatable because the game is
    paused between steps. Blind to fps60 interpolated frames by construction — use preseq for those."""
    os.makedirs(directory, exist_ok=True)
    for i in range(n):
        dbg("step 1")
        dbg(f"shot {directory}/f{i:03d}.png")
    out(f"[sandbox] captured {n} logic frames -> {directory}/f000..f{n-1:03d}.png")


def run_scenario(dbg, lines, transcript):
    def out(s):
        print(s)
        transcript.append(s)

    for raw in lines:
        # Only a LINE-LEADING '#' is a comment. An inline '#' is data: `echo kanban #71` must keep
        # its issue number, and guest addresses/args are routinely written with '#'.
        line = "" if raw.lstrip().startswith("#") else raw.strip()
        if not line:
            continue
        verb, _, rest = line.partition(" ")
        rest = rest.strip()
        if verb == "echo":
            out(f"[sandbox] {rest}")
        elif verb == "sleep":
            time.sleep(float(rest))
            out(f"[sandbox] slept {rest}s")
        elif verb == "spawn":
            do_spawn(dbg, rest, out)
        elif verb == "capture":
            parts = rest.split()
            n = int(parts[0])
            directory = parts[1] if len(parts) > 1 else f"{REPO}/scratch/screenshots/sandbox"
            do_capture(dbg, n, directory, out)
        else:
            reply = dbg(line).strip()
            out(f"> {line}\n{reply}" if reply else f"> {line}")
    return transcript


def sheet(directory, cols=4):
    """Contact-sheet a frame directory. Refuses on a missing/empty directory rather than producing
    an empty sheet that would read as 'nothing happened'."""
    try:
        from PIL import Image
    except ImportError:
        sys.exit("sandbox --sheet needs Pillow (pip install pillow)")
    if not os.path.isdir(directory):
        sys.exit(f"sandbox --sheet: {directory} DOES NOT EXIST — it searched nothing, not 'found nothing'")
    files = sorted(f for f in os.listdir(directory) if f.lower().endswith((".png", ".ppm")))
    if not files:
        sys.exit(f"sandbox --sheet: {directory} exists but holds 0 png/ppm frames (scanned "
                 f"{len(os.listdir(directory))} entries)")
    ims = [Image.open(os.path.join(directory, f)).convert("RGB") for f in files]
    w, h = ims[0].size
    rows = (len(ims) + cols - 1) // cols
    out = Image.new("RGB", (w * cols, h * rows))
    for i, im in enumerate(ims):
        out.paste(im, ((i % cols) * w, (i // cols) * h))
    path = os.path.join(directory, "sheet.png")
    out.save(path)
    print(f"[sandbox] {len(ims)} frames ({w}x{h}) -> {path}")
    return path


def selftest():
    """Prove the scenario parser and the spawn-failure gate actually FIRE, with no game running.
    A self-test nobody runs is the same bug one level up, so this is the shipping artifact's own."""
    ok = True
    log = []

    class FakeDbg:
        def __init__(self, v0):
            self.v0, self.seen = v0, []

        def __call__(self, line):
            self.seen.append(line)
            if line.startswith("call"):
                return f"call ... -> v0={self.v0} v1=00000000\n"
            return ""

    # 1. A GOOD spawn resolves the recipe to the game's own address and reports the node.
    d = FakeDbg("800FB218")
    run_scenario(d, ["# comment", "", "echo hi", "spawn banner:red-chest", "press right"], log)
    want = f"call {0x80040AA4:X} 38 0"
    if want not in d.seen:
        print(f"SELFTEST FAIL: recipe did not resolve to {want!r}; sent {d.seen}"); ok = False

    # 2. A REFUSED spawn (v0=0) must be FATAL. This is the branch that keeps a scenario which
    #    spawned nothing from producing a clean-looking frame series.
    d0 = FakeDbg("00000000")
    try:
        run_scenario(d0, ["spawn banner:red-chest"], [])
        print("SELFTEST FAIL: v0=0 spawn was NOT treated as a failure"); ok = False
    except SystemExit as e:
        if "SPAWN FAILED" not in str(e):
            print(f"SELFTEST FAIL: wrong error for v0=0: {e}"); ok = False

    # 3. An unknown recipe must be refused, not silently skipped.
    try:
        run_scenario(FakeDbg("1"), ["spawn nope:nope"], [])
        print("SELFTEST FAIL: unknown recipe was accepted"); ok = False
    except SystemExit:
        pass

    # 4. Unknown verbs pass through verbatim (the whole debug-server surface stays reachable).
    d2 = FakeDbg("1")
    run_scenario(d2, ["rw 801fe00c 4", "tp 100 200 300"], [])
    if "rw 801fe00c 4" not in d2.seen or "tp 100 200 300" not in d2.seen:
        print(f"SELFTEST FAIL: passthrough dropped a command; sent {d2.seen}"); ok = False

    print(f"selftest: {'PASS (4/4 checks)' if ok else 'FAIL'}")
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("scenario", nargs="?", help="scenario file to run")
    ap.add_argument("--port", type=int, default=5970, help="debug-server port to launch on (never 5959)")
    ap.add_argument("--attach", type=int, help="drive an instance already listening on this port")
    ap.add_argument("--list", action="store_true", help="list the effect recipes and their RE sources")
    ap.add_argument("--selftest", action="store_true", help="prove the parser fires; no game needed")
    ap.add_argument("--sheet", help="contact-sheet a directory of captured frames and exit")
    ap.add_argument("--keep", action="store_true", help="leave the launched instance running")
    a = ap.parse_args()

    if a.selftest:
        return selftest()
    if a.list:
        for name in sorted(RECIPES):
            r = RECIPES[name]
            print(f"{name}\n    {r['what']}\n    call {r['addr']:#X}({', '.join(map(str, r['args']))})"
                  f"\n    source: {r['src']}\n")
        return 0
    if a.sheet:
        sheet(a.sheet)
        return 0
    if not a.scenario:
        ap.error("give a scenario file, or --list / --selftest / --sheet")

    lines = open(a.scenario).read().splitlines()
    proc = None
    port = a.attach or a.port
    if not a.attach:
        proc = launch(port)
    try:
        dbg = Dbg(port)
        run_scenario(dbg, lines, [])
    finally:
        if proc and not a.keep:
            proc.kill()
            print(f"[sandbox] killed pid={proc.pid}")
        elif proc:
            print(f"[sandbox] instance left running: pid={proc.pid} port={port}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
