#!/usr/bin/env python3
"""tomba1_live.py — drive the SHIPPING Tomba! 1 product over its loopback control channel.

WHAT THIS IS, AND WHAT IT IS NOT. This is a transport plus a report. It connects to the product's
own debug endpoint, waits for REAL PRESENTED frames, delivers a real pad edge, reads guest state
back, and captures a screenshot. It deliberately contains NO knowledge of which screen is on display:
`tools/title_prompts.py` owns that for Tomba! 2 and nothing here guesses at a Tomba! 1 equivalent, so
this reports what the guest's own memory says and lets the reader judge the picture.

IT CANNOT CLAIM GAMEPLAY. The title's recorded frontier (docs/issues/0008) is a code-image-identity
fault after boot, so a run driven here reaches boot presentation and stops. A run of this tool
reaching a picture is evidence that the product presents and that input is delivered, and is NOT
evidence that Tomba! 1 reaches representative gameplay. The report says so in its own output rather
than leaving it to the reader.

REFUSALS, never a degraded run (exit 2, never 0):
  - the endpoint never answers;
  - the presented-frame counter never advances, which would make every later number meaningless;
  - the screenshot is not written, so nothing claims to have seen a picture.
"""

from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "external" / "psxport" / "tools"))

from dbgclient import LiveClient  # noqa: E402  (the framework's one protocol implementation)

DEFAULT_PORT = 5971
# Tomba! 1's own immutable executable facts; the driver reads guest memory, it does not guess.
K_CURRENT_TASK = 0x1F8001D4
K_PAD_SLOT0 = 0x8009EB58
K_PAD_SLOT1 = 0x8009EB7A
# The four guest task records: base 0x801FD800, stride 0x70.
K_TASK_TABLE = 0x801FD800
K_TASK_STRIDE = 0x70
K_TASK_SLOTS = 3


def launch(binary: Path, disc: str, port: int, log: Path) -> subprocess.Popen:
    env = dict(os.environ)
    env["PSXPORT_TOMBA1_DISC"] = disc
    env["PSXPORT_ASSET_DIR"] = str(REPO / "external" / "psxport")
    env["PSXPORT_DEBUG_SERVER"] = str(port)
    env["PSXPORT_NATIVE_FRAMES"] = "1000000"
    env["PSXPORT_WATCHDOG"] = "0"
    env["PSXPORT_NOPACE"] = "1"
    log.parent.mkdir(parents=True, exist_ok=True)
    handle = log.open("w")
    return subprocess.Popen([str(binary)], cwd=REPO, env=env, stdout=handle, stderr=subprocess.STDOUT)


def connect(port: int, deadline_seconds: float) -> LiveClient | None:
    """The endpoint is absent until the product's own boot has attached it, and this title's boot
    takes seconds. Polling to a deadline, then REFUSING, is the honest alternative to reporting a
    zero-frame run as a successful one."""
    end = time.monotonic() + deadline_seconds
    while time.monotonic() < end:
        try:
            return LiveClient(port, timeout=30.0)
        except OSError:
            time.sleep(0.25)
    return None


def read_task_states(client: LiveClient) -> list[tuple[int, int, int]]:
    """(state, retry counter, entry) for each of the guest's three task records.

    Reported as a whole table with its denominator rather than a single slot: a reader asking "is any
    task running" needs all three, and a driver that printed one slot could be showing a parked task
    and calling it progress."""
    rows: list[tuple[int, int, int]] = []
    for slot in range(K_TASK_SLOTS):
        base = K_TASK_TABLE + slot * K_TASK_STRIDE
        words = client.words(base, 4)
        rows.append((words[0] & 0xFFFF, words[3] & 0xFFFF, words[2]))
    return rows


def product_fatal(log: Path) -> str:
    """The product's own fatal line, if it has one.

    A driver whose product has already aborted must say so with the PRODUCT's reason. Letting the
    socket error surface instead reports a transport failure for a game that stopped on its own, which
    is the same class of error as reading a counter nothing feeds."""
    if not log.is_file():
        return "(no product log)"
    for line in reversed(log.read_text(errors="replace").splitlines()):
        if ":error]" in line or "FATAL" in line:
            return line.split("] ", 1)[-1][:200]
    return "(product exited without an error line)"


def product_stopped(process: subprocess.Popen, log: Path) -> str:
    """One line describing the product's OBSERVED state, for a refusal or a report.

    `poll()` returning None means "this process object has not been reaped yet", which is NOT the
    same claim as "the product is running" — it is also what a process that is dying of SIGSEGV looks
    like for a few milliseconds. Reporting the first as the second is a confident wrong answer about
    the one thing the reader most needs to trust, so the wording says which of the two was actually
    observed, and a dropped connection is named as a dropped connection rather than as a live
    product."""
    code = process.poll()
    if code is not None:
        return f"the product exited with code {code}: {product_fatal(log)}"
    return ("the product had NOT been reaped when this line was written, and the control channel had "
            f"already dropped the connection; its last logged state is: {product_fatal(log)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", default=os.environ.get("PSXPORT_TOMBA1_DISC", ""))
    parser.add_argument("--binary", type=Path, default=REPO / "build" / "bin" / "tomba1_port")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--out", type=Path, default=REPO / "scratch" / "live" / "tomba1")
    args = parser.parse_args()
    if not args.disc:
        print("REFUSED: --disc (or PSXPORT_TOMBA1_DISC) is required; this title refuses generic disc "
              "fallbacks by design", file=sys.stderr)
        return 2
    if not args.binary.is_file():
        print(f"REFUSED: {args.binary} does not exist — NOTHING WAS RUN", file=sys.stderr)
        return 2

    args.out.mkdir(parents=True, exist_ok=True)
    log = args.out / "product.log"
    shot = args.out / "boot.ppm"
    process = launch(args.binary, args.disc, args.port, log)
    print(f"[live] launched pid={process.pid} log={log}")

    client = connect(args.port, 60.0)
    if client is None:
        process.send_signal(signal.SIGTERM)
        print("REFUSED: the product's control channel never answered within 60 s — the run drove "
              "nothing, so every number below would be about a game that never ran", file=sys.stderr)
        return 2

    try:
        # ORDER IS THE POINT HERE, and it is not stylistic. This title's whole remaining life is six
        # presented frames: the boot decompress alone spans seven display fields (docs/issues/0007)
        # and the recorded code-image-identity fault lands immediately after it. So the picture is
        # taken BEFORE any input, because waiting to observe a state change first would spend the only
        # window the product has and leave no screenshot at all. A driver that captured its picture
        # last would report "no picture" on exactly the runs where a picture matters most.
        try:
            frames = client.frames()
        except (RuntimeError, OSError) as error:
            print(f"REFUSED: {product_stopped(process, log)} — the control channel could not report a "
                  f"presented-frame counter ({error})", file=sys.stderr)
            return 2
        if frames.get("frame", 0) <= 0:
            print("REFUSED: the product presented ZERO real frames; nothing below is evidence about "
                  "the game", file=sys.stderr)
            return 2
        print(f"[live] presented frames at first sample: real={frames['frame']} "
              f"in-between={frames.get('interp', 0)} total={frames['total']}")

        shot_reply = ""
        try:
            shot_reply = client.shot(str(shot)).strip()
        except (RuntimeError, OSError) as error:
            shot_reply = f"unavailable: {product_stopped(process, log)} ({error})"
        if shot.is_file() and shot.stat().st_size > 0:
            print(f"[live] FIRST screenshot {shot} ({shot.stat().st_size} bytes); reply: {shot_reply}")
        else:
            print(f"REFUSED: `shot {shot}` replied {shot_reply!r} but wrote no picture; no line here "
                  f"is a claim about what the screen showed", file=sys.stderr)
            return 2

        # Real pad input, held across presented frames so the edge spans a guest sample.
        try:
            before = client.words(K_PAD_SLOT0, 2)
            print(f"[live] pad slot0 before tap: {before[0]:04X} {before[1]:04X}")
            print(f"[live] tap Start across 6 presented frames -> {client.tap('start', 6).strip()}")
            time.sleep(1.5)
            after = client.words(K_PAD_SLOT0, 2)
        except (RuntimeError, OSError) as error:
            print(f"[live] pad sample unavailable: {product_stopped(process, log)} ({error})")
        else:
            print(f"[live] pad slot0 after tap:  {after[0]:04X} {after[1]:04X}")
            moved = before != after
            print(f"[live] guest pad state CHANGED across the tap: {moved} "
                  f"({'input reached guest memory' if moved else 'NO CHANGE — input did not reach the guest'})")

        try:
            for slot, (state, retry, entry) in enumerate(read_task_states(client)):
                print(f"[live] guest task {slot}: state={state} retry={retry} entry=0x{entry:08X}")
            print(f"[live] current task record [0x1F8001D4] = 0x{client.word(K_CURRENT_TASK):08X}")
        except (RuntimeError, OSError) as error:
            print(f"[live] task-table sample unavailable: {product_stopped(process, log)} ({error})")

        # Honest non-claim, in the run's own output rather than only in this file's docstring.
        print("[live] THIS RUN IS NOT GAMEPLAY, and the product does not survive long enough for it to "
              "be: it presented the frames counted above and then stopped on its recorded "
              "code-image-identity fault (docs/issues/0008). Open the screenshot and judge it before "
              "quoting anything above.")
        # Reap before describing, so this line reports an exit code when there is one.
        for _ in range(30):
            if process.poll() is not None:
                break
            time.sleep(0.1)
        print(f"[live] product state at report time: {product_stopped(process, log)}")
        return 0
    finally:
        try:
            client.quit()
        except OSError:
            pass
        for _ in range(50):
            if process.poll() is not None:
                break
            time.sleep(0.1)
        if process.poll() is None:
            process.kill()
        print(f"[live] product exit code: {process.returncode}")


if __name__ == "__main__":
    raise SystemExit(main())
