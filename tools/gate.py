#!/usr/bin/env python3
"""gate.py — the AGENT's headless gate. Never `./run.sh`.

WHY THIS EXISTS (USER, 2026-08-11: "I don't know why you are running run.sh, that is for me to play,
you should have your own tools"). `run.sh` is the user's end-to-end WINDOWED play launcher: it re-syncs
submodules, re-extracts the boot executable, rebuilds, and launches. An agent invoking it competes with
the user's session for the shared tree, and its submodule re-sync can silently revert in-progress
framework work to the recorded pin — after which every measurement describes a different framework than
the agent thinks (`external/psxport/docs/workspace/PROTOCOL.md` records that incident). So agents drive
the ALREADY-BUILT binary, and that is what this does.

It does NOT build and does NOT extract. Build with:
    cmake --build build --target tomba2_port -j$(nproc)

WHAT A PASS MEANS, STATED SO IT CANNOT BE OVERREAD. A green gate here means: the binary launched
headless, the REPL accepted the script, the run reached the asserted frame, and nothing matching the
failure patterns appeared. It says NOTHING about pixels or about SBS byte-exactness. Every run prints
its own denominator — frames reached, lines scanned, patterns searched — because a gate that prints
only "OK" is indistinguishable from one that never ran the game (CLAUDE.md: "a diagnostic that can
print nothing is lying").

REFUSALS (exit 2, never 0): a missing binary, a missing boot executable, a REPL that produced no
output, or a run whose frame counter never appeared. Each says what it did NOT do rather than
returning a clean empty pass.

USAGE
    python3 tools/gate.py boot [--frames 400] [--expect-frame 440]
    python3 tools/gate.py replay <replays/.../foo.pad> [--frames 900]
    python3 tools/gate.py run --script 'newgame
run 400
stage
quit' [--debug nofx,plumefx]
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, 'scratch', 'bin', 'tomba2_port')
EXE = os.path.join(REPO, 'scratch', 'bin', 'tomba2', 'MAIN.EXE')
LOGDIR = os.path.join(REPO, 'scratch', 'logs')

# Anything here in the output fails the gate. A recomp MISS aborts the process by design, but it can
# also appear in a line that scrolls past a watchdog kill, so it is matched as text too.
FAIL_PATTERNS = [
    r'\bFATAL\b',
    r'\babort\b',
    r'Aborted',
    r'recomp[- ]MISS',
    r'rec_dispatch miss',
    r'\bASSERT\b',
    r'Segmentation fault',
    r'std::bad_alloc',
]
# The cfg subsystem names a knob that matched nothing. Not fatal, but it means a flag the caller was
# relying on did NOTHING, so it is surfaced loudly rather than buried.
UNKNOWN_KNOB = re.compile(r'UNKNOWN knob (\S+)')
FRAME_RE = re.compile(r'frame[= ](\d+)')
# `newgame` pulses an unspecified number of frames to reach the GAME prologue, so the ABSOLUTE frame a
# run ends on is a function of how long that took (measured: 27, while a note from an earlier build says
# 39). Asserting an absolute frame is therefore hardcoding an expected value — a bandaid that fails for
# a reason having nothing to do with the change under test. Assert the ADVANCE past the prologue, and
# the END STATE, which are the actual invariants.
PROLOGUE_RE = re.compile(r'reached GAME prologue at frame (\d+)')
ENDSTATE_RE = re.compile(r'stage=([0-9A-Fa-f]+)\s+sm48=(\d+)')
ENV_AUDIT_RE = re.compile(r'env audit AT EXIT[^\n]*')


def refuse(msg: str) -> int:
    print(f"GATE REFUSED: {msg}", file=sys.stderr)
    return 2


def run_gate(script: str, frames_hint: int, debug: str, watchdog: int,
             expect_frame: int, extra_env: dict, label: str,
             expect_stage: str = '', expect_sm48: str = '') -> int:
    if not os.path.isfile(BIN):
        return refuse(f"{BIN} does not exist — NOTHING WAS RUN. Build first: "
                      f"cmake --build build --target tomba2_port -j$(nproc). Do not read this as a pass.")
    if not os.path.isfile(EXE):
        return refuse(f"{EXE} does not exist — NOTHING WAS RUN. The boot executable is extracted from "
                      f"the disc; it is not this tool's job to extract it (that is run.sh's, and run.sh "
                      f"belongs to the user). Extract it once, then re-run this gate.")

    env = dict(os.environ)
    # NOT setting PSXPORT_VK_HEADLESS: the binary is headless BY DEFAULT (gpu_vk.cpp requires
    # PSXPORT_VK_WINDOW=1 to open a window, precisely so an agent that forgets a flag fails safe), and
    # setting it only produces a known-false "UNKNOWN knob" line — gpu_vk does not initialise until
    # after the startup validator has run (external/psxport/runtime/recomp/config.cpp:386).
    env['PSXPORT_NOPACE'] = '1'
    env['PSXPORT_NOAUDIO'] = '1'
    env['PSXPORT_REPL'] = '1'
    env['PSXPORT_WATCHDOG'] = str(watchdog)
    if debug:
        env['PSXPORT_DEBUG'] = debug
    env.update(extra_env)

    os.makedirs(LOGDIR, exist_ok=True)
    stamp = time.strftime('%Y%m%d-%H%M%S')
    logpath = os.path.join(LOGDIR, f'gate-{label}-{stamp}.log')

    t0 = time.time()
    try:
        p = subprocess.run([BIN, EXE], input=script, capture_output=True, text=True,
                           env=env, cwd=REPO, timeout=watchdog + 120)
    except subprocess.TimeoutExpired:
        return refuse(f"the binary did not exit within {watchdog + 120}s even though PSXPORT_WATCHDOG="
                      f"{watchdog}. The watchdog itself did not fire — treat this as a HANG, not a pass.")
    dt = time.time() - t0
    out = (p.stdout or '') + (p.stderr or '')
    with open(logpath, 'w') as f:
        f.write(out)

    lines = out.count('\n')
    if lines == 0:
        return refuse(f"the binary produced ZERO output lines in {dt:.1f}s (exit {p.returncode}). "
                      f"Nothing was observed, so nothing is proven. Log: {logpath}")

    frames = [int(m.group(1)) for m in FRAME_RE.finditer(out)]
    maxframe = max(frames) if frames else -1
    mp = PROLOGUE_RE.search(out)
    prologue = int(mp.group(1)) if mp else -1
    me = ENDSTATE_RE.search(out)
    end_stage = me.group(1).upper() if me else ''
    end_sm48 = me.group(2) if me else ''
    audit = ENV_AUDIT_RE.search(out)

    hits = []
    for pat in FAIL_PATTERNS:
        for m in re.finditer(pat, out, re.IGNORECASE):
            ctx = out[max(0, m.start() - 100):m.end() + 140].replace('\n', ' | ')
            hits.append((pat, ctx))
            break

    # PSXPORT_REPL is read via the legacy cfg_on() path (native_boot.cpp:299) but is not registered in
    # config_vars.h, so the startup validator reports it UNKNOWN even though it works. Filtered so a
    # REAL unknown knob — a flag the caller is relying on that did nothing — is not lost in noise.
    # Verified rather than assumed: the REPL demonstrably drives the run (a `run N` advances the frame
    # counter by N past the newgame prologue).
    KNOWN_FALSE_UNKNOWN = {'PSXPORT_REPL'}
    unknown = sorted(set(UNKNOWN_KNOB.findall(out)) - KNOWN_FALSE_UNKNOWN)

    # ---- the report, always with its denominator ------------------------------------------------
    print(f"[gate:{label}] exit={p.returncode} in {dt:.1f}s · {lines} output line(s) · "
          f"max frame counter seen = {maxframe if maxframe >= 0 else 'NONE'}")
    print(f"[gate:{label}] scanned {lines} line(s) against {len(FAIL_PATTERNS)} failure pattern(s): "
          f"{', '.join(FAIL_PATTERNS)}")
    if prologue >= 0:
        print(f"[gate:{label}] newgame prologue at frame {prologue}; advanced "
              f"{maxframe - prologue} frame(s) past it"
              + (f" (asked for {frames_hint})" if frames_hint else ""))
    if end_stage:
        print(f"[gate:{label}] end state: stage={end_stage} sm48={end_sm48}")
    if audit:
        print(f"[gate:{label}] {audit.group(0).strip()}")
    print(f"[gate:{label}] log: {logpath}")
    if unknown:
        print(f"[gate:{label}] WARNING: {len(unknown)} knob(s) set but matched NOTHING — they did "
              f"nothing this run: {', '.join(unknown)}")

    bad = 0
    if hits:
        print(f"[gate:{label}] FAIL — {len(hits)} failure pattern(s) matched:")
        for pat, ctx in hits:
            print(f"    /{pat}/  …{ctx}…")
        bad = 1
    if maxframe < 0:
        print(f"[gate:{label}] FAIL — no frame counter ever appeared in the output, so the run cannot "
              f"be said to have advanced. This is not a pass.")
        bad = 1
    elif expect_frame and maxframe < expect_frame:
        print(f"[gate:{label}] FAIL — reached frame {maxframe}, expected at least {expect_frame}.")
        bad = 1
    # The real assertion: did the run actually ADVANCE the number of frames it was told to, past the
    # prologue? This is what an absolute frame floor was standing in for, without the magic constant.
    if frames_hint and prologue >= 0 and (maxframe - prologue) < frames_hint:
        print(f"[gate:{label}] FAIL — asked to advance {frames_hint} frame(s) past the prologue "
              f"(frame {prologue}) but only reached {maxframe}, i.e. {maxframe - prologue}.")
        bad = 1
    if expect_stage and end_stage != expect_stage.upper():
        print(f"[gate:{label}] FAIL — end stage {end_stage or 'NONE REPORTED'}, expected "
              f"{expect_stage.upper()}.")
        bad = 1
    if expect_sm48 and end_sm48 != expect_sm48:
        print(f"[gate:{label}] FAIL — end sm48 {end_sm48 or 'NONE REPORTED'}, expected {expect_sm48}.")
        bad = 1
    if p.returncode != 0 and not hits:
        print(f"[gate:{label}] FAIL — non-zero exit {p.returncode} with no failure pattern matched; "
              f"read the log, the pattern list is incomplete for this failure.")
        bad = 1

    if bad:
        return 1
    print(f"[gate:{label}] PASS — launched headless, REPL accepted the script, advanced "
          f"{maxframe - prologue if prologue >= 0 else maxframe} frame(s)"
          + (f" to stage={end_stage} sm48={end_sm48}" if end_stage else "")
          + f", no failure pattern matched. This says NOTHING about pixels or SBS parity.")
    return 0


def cmd_boot(args) -> int:
    script = f"newgame\nrun {args.frames}\nstage\nquit\n"
    return run_gate(script, args.frames, args.debug, args.watchdog, args.expect_frame, {}, 'boot',
                    args.expect_stage, args.expect_sm48)


def cmd_replay(args) -> int:
    pad = args.pad if os.path.isabs(args.pad) else os.path.join(REPO, args.pad)
    if not os.path.isfile(pad):
        return refuse(f"replay {pad} does not exist — NOTHING WAS RUN.")
    script = f"newgame\nrun {args.frames}\nquit\n"
    return run_gate(script, args.frames, args.debug, args.watchdog, args.expect_frame,
                    {'PSXPORT_SBS_PAD_REPLAY': pad}, 'replay')


def cmd_run(args) -> int:
    if not args.script:
        return refuse("--script is empty — NOTHING WAS RUN.")
    return run_gate(args.script if args.script.endswith('\n') else args.script + '\n',
                    0, args.debug, args.watchdog, args.expect_frame, {}, 'run')


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--debug', default='', help='PSXPORT_DEBUG channels, e.g. nofx,plumefx')
    ap.add_argument('--watchdog', type=int, default=120, help='PSXPORT_WATCHDOG seconds (default 120)')
    ap.add_argument('--expect-frame', type=int, default=0,
                    help='fail unless the ABSOLUTE frame counter reaches this. Prefer the defaults: the '
                         'gate already asserts the advance past the prologue, which is the same '
                         'invariant without a magic constant')
    ap.add_argument('--expect-stage', default='',
                    help='fail unless the run ends on this task-0 stage entry, e.g. 8010637C')
    ap.add_argument('--expect-sm48', default='',
                    help='fail unless the run ends with this stage state-machine value, e.g. 2')
    sub = ap.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('boot', help='newgame + run N frames headless')
    b.add_argument('--frames', type=int, default=400)
    b.set_defaults(fn=cmd_boot)
    r = sub.add_parser('replay', help='run a recorded pad replay headless')
    r.add_argument('pad')
    r.add_argument('--frames', type=int, default=900)
    r.set_defaults(fn=cmd_replay)
    x = sub.add_parser('run', help='drive an arbitrary REPL script')
    x.add_argument('--script', required=True)
    x.set_defaults(fn=cmd_run)
    args = ap.parse_args()
    return args.fn(args)


if __name__ == '__main__':
    sys.exit(main())
