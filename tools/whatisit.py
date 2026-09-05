#!/usr/bin/env python3
"""whatisit.py — answer "WHAT IN THE GAME am I actually porting?" for a guest address.

WHY THIS EXISTS. This project names things from the disassembly, and it shows: all 46 live field
handlers carry STRUCTURAL names (substate_edge_orchestrator, scatter_record_dither,
jumptable_release_trigger) rather than game names. Ports get accurate mechanics and a plausible label,
and nobody has looked at the screen. The USER asked, bluntly, whether I knew what I was porting; the
honest answer was "the mechanics yes, the object no". This tool closes that gap and makes closing it
cheap enough that there is no excuse next time.

WHAT IT DOES. Drives a replay headless to a frame, walks BOTH entity lists (`ents`), captures a
screenshot at the SAME instant, and prints the live nodes with their world positions, their distance
from the player, and — via codemap — who owns each handler natively. Then you LOOK at the picture.

    tools/whatisit.py 0x80118B10                       # which live node runs (or reaches) this?
    tools/whatisit.py --all                            # the whole live node set, nearest first
    tools/whatisit.py 0x80118B10 --frame 1400 --replay replays/bugs/seesaw-weight.pad

THE HONEST LIMIT, and it is the important part. A LEAF address usually does NOT appear as any node's
handler — only top-level per-object behaviours do. When that happens this tool says so explicitly and
falls back to showing the nodes near the player, because a leaf's identity has to come from its
CALLER's node. It will never silently print an empty table and let that read as "nothing found":
a search that matched nothing says how many nodes it examined and what it could not see.

AND IT DOES NOT NAME ANYTHING. It gives you positions and a picture. An offset is a fact, a position is
a fact, a NAME is a claim that needs a source (the USER, or guest data) — the same rule docs/areas.md
applies to areas. Do not let a plausible-looking screenshot become an identification.
"""
import argparse, os, re, subprocess, sys, math

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTS_RE = re.compile(
    r'^\[ents\]\s+([0-9A-F]{8})\s+t=(\S+)\s+ri=(\S+)\s+model=(\S+)\s+h=([0-9A-F]{8})\s+'
    r'pos=\(\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\)\s+rf=(\d+)\s+cmds=(\d+)\s+\S+\s*(\S*)')


def run_game(replay, frame, shot):
    """Drive the replay to `frame`, dump ents, and grab a PNG at the same instant."""
    cmds = [f"skip {frame}", "ents", f"shot {shot}", "quit"]
    # PSXPORT_NOPACE: a probe, not a play session. Headless is PACED like a windowed run now (they
    # are one program), so "as fast as the host can" has to be ASKED for.
    env = dict(os.environ, PSXPORT_REPL="1", PSXPORT_VK_HEADLESS="1", PSXPORT_NOAUDIO="1",
                           PSXPORT_NOPACE="1",
               PSXPORT_PAD_REPLAY=replay)
    binp = os.path.join(ROOT, "build/bin/tomba2_port")
    if not os.path.isfile(binp):
        sys.exit(f"whatisit: {binp} does not exist — build first "
                 f"(cmake --build build --target tomba2_port). Refusing rather than reporting nothing.")
    p = subprocess.run([binp], input="\n".join(cmds) + "\n", capture_output=True, text=True,
                       env=env, cwd=ROOT, timeout=900)
    return p.stdout + p.stderr


_OWNER_CACHE = {}


def owner_of(addr):
    """Who owns this guest address natively, per codemap — the authoritative resolver, not a grep.

    MEMOISED, because codemap --addr re-scans ~1000 natives on every call and many nodes share a
    handler; without the cache a 14-row table meant 14 full tree scans and the tool took minutes."""
    if addr in _OWNER_CACHE:
        return _OWNER_CACHE[addr]
    try:
        out = subprocess.run([sys.executable, os.path.join(ROOT, "tools/codemap.py"), "--addr", addr],
                             capture_output=True, text=True, cwd=ROOT, timeout=120).stdout
    except Exception:
        _OWNER_CACHE[addr] = "(codemap failed)"
        return _OWNER_CACHE[addr]
    m = re.search(r'^0x\S+:\s+(.+?)\s+\[(LIVE|ORPHAN)\]', out, re.M)
    if m:
        res = f"{m.group(1)} [{m.group(2)}]"
    elif "OWNED by PlatformHle" in out:
        res = "PlatformHle"
    else:
        res = "(unowned)"
    _OWNER_CACHE[addr] = res
    return res


def main():
    ap = argparse.ArgumentParser(description="what in the game is this guest address?")
    ap.add_argument("addr", nargs="?", help="guest address, e.g. 0x80118B10")
    ap.add_argument("--replay", default="replays/bugs/seesaw-weight.pad")
    ap.add_argument("--frame", type=int, default=1400)
    ap.add_argument("--shot", default="scratch/screenshots/whatisit.png")
    ap.add_argument("--all", action="store_true", help="list every live node, nearest the player first")
    a = ap.parse_args()
    if not a.addr and not a.all:
        ap.error("give an address, or --all")

    os.makedirs(os.path.join(ROOT, os.path.dirname(a.shot)), exist_ok=True)
    log = run_game(a.replay, a.frame, a.shot)

    nodes = []
    for line in log.splitlines():
        m = ENTS_RE.match(line.strip())
        if m:
            nodes.append(dict(addr=m.group(1), t=m.group(2), model=m.group(4), h=m.group(5),
                              x=int(m.group(6)), y=int(m.group(7)), z=int(m.group(8)),
                              rf=int(m.group(9)), cmds=int(m.group(10)), name=m.group(11)))
    if not nodes:
        sys.exit("whatisit: the entity walk produced NO nodes. That is a tool failure, not a finding — "
                 "check the replay path and that the frame is past area load. Refusing to report an "
                 "empty result as an answer.")

    player = next((n for n in nodes if "camera_target_follow" in n["name"]), None)
    for n in nodes:
        n["dist"] = (math.dist((n["x"], n["y"], n["z"]), (player["x"], player["y"], player["z"]))
                     if player else 0.0)

    print(f"frame {a.frame} of {a.replay} — {len(nodes)} live nodes, screenshot {a.shot}")
    if player:
        print(f"player at ({player['x']}, {player['y']}, {player['z']})   "
              f"(Y grows DOWNWARD: more negative is HIGHER)")

    want = a.addr.lower().replace("0x", "") if a.addr else None
    if want:
        hits = [n for n in nodes if n["h"].lower() == want]
        if hits:
            print(f"\n{len(hits)} live node(s) run 0x{want.upper()} as their own handler:")
            rows = hits
        else:
            print(f"\nNO live node has 0x{want.upper()} as its handler. That is EXPECTED for a LEAF — "
                  f"only top-level per-object behaviours appear here, so a leaf's identity comes from "
                  f"its CALLER's node. Examined {len(nodes)} nodes across both entity lists.")
            print("Showing the nodes nearest the player instead; find your caller among them.")
            rows = sorted(nodes, key=lambda n: n["dist"])[:14]
    else:
        rows = sorted(nodes, key=lambda n: n["dist"])

    print(f"\n{'node':>8} {'h':>8} {'pos':>26} {'dist':>7} {'vis':>3} {'name':<32} owner")
    for n in rows:
        pos = f"({n['x']}, {n['y']}, {n['z']})"
        print(f"{n['addr']:>8} {n['h']:>8} {pos:>26} {n['dist']:>7.0f} {n['rf']:>3} "
              f"{n['name'][:32]:<32} {owner_of('0x' + n['h'])}")
    print(f"\nNow LOOK at {a.shot} and match by position. Do NOT name the object from the picture "
          f"alone — a position is a fact, a name is a claim that needs a source.")


if __name__ == "__main__":
    main()
