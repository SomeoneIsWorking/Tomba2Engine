#!/usr/bin/env python3
"""drive_to_grab.py — reach and observe the RIDE/GRAB state under ANY config, closed-loop.

WHY THIS EXISTS
---------------
A recorded .pad is OPEN-LOOP: it replays button masks against frame indices. The moment a change
alters behaviour — a fix, PSXPORT_PC_SKIP=0, PSXPORT_GATE=1 — the world diverges and the same frame
no longer corresponds to the same game moment. Every attempt to A/B the water-pump seesaw (kanban #8)
died on this: the "after" run simply was not grabbing anything at the frame the "before" run was, so
the comparison measured nothing. Frame numbers are not a game state.

This driver closes the loop. It watches the RIDE/ATTACH POINTER at G+0x158 (0x800E7FD8) — the object
Tomba is holding, 0 when he holds nothing — and reports what the beam does WHILE THAT IS NON-ZERO.
The pad replay is used only to get near the pump; the measurement is gated on observed state, so it
stays valid under any configuration.

USAGE
    tools/drive_to_grab.py [--port 5960] [--replay replays/bugs/seesaw-weight.pad]
                           [--timeout 240] [--env KEY=VAL ...]

    # the two comparisons that matter for #8:
    tools/drive_to_grab.py --env PSXPORT_PC_SKIP=1
    tools/drive_to_grab.py --env PSXPORT_PC_SKIP=0

Prints, once the attach pointer goes non-zero: the held object, its node[0x2b] (contact stamp),
node[0x48] (the weight), and its tilt angle over the following samples — the four numbers that decide
whether the seesaw is working. Exit code 0 if a grab was observed, 2 if it never happened.
"""
import argparse, os, socket, subprocess, sys, time

BIN = "./scratch/bin/tomba2_port"
ATTACH_PTR = 0x800E7FD8          # G+0x158: the object Tomba is holding (0 = nothing)


def cmd(port, line, retries=1):
    """One debug-server command. Returns '' if the server is not up yet."""
    for _ in range(retries + 1):
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=5)
            s.sendall((line + "\n").encode())
            out = b""
            s.settimeout(5)
            try:
                while True:
                    b = s.recv(4096)
                    if not b:
                        break
                    out += b
            except socket.timeout:
                pass
            s.close()
            return out.decode(errors="replace")
        except OSError:
            time.sleep(0.5)
    return ""


def word(port, addr):
    """Read one guest word; None if unreadable."""
    out = cmd(port, "rw %08X 1" % addr)
    for tok in out.replace(":", " ").split():
        if len(tok) == 8:
            try:
                v = int(tok, 16)
                if v != addr:
                    return v
            except ValueError:
                pass
    return None


def byte_at(port, addr):
    out = cmd(port, "r %08X 1" % addr)
    parts = out.split(":")
    if len(parts) < 2:
        return None
    try:
        return int(parts[1].split()[0], 16)
    except (ValueError, IndexError):
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=5960)
    ap.add_argument("--replay", default="replays/bugs/seesaw-weight.pad")
    ap.add_argument("--timeout", type=float, default=240.0)
    ap.add_argument("--samples", type=int, default=12, help="state samples to take once grabbed")
    ap.add_argument("--env", action="append", default=[], metavar="K=V")
    a = ap.parse_args()

    env = dict(os.environ)
    env.update({
        "PSXPORT_NOWINDOW": "1", "PSXPORT_NOAUDIO": "1", "PSXPORT_NO_FMV": "1",
        "PSXPORT_NATIVE_FRAMES": "100000", "PSXPORT_DEBUG_SERVER": str(a.port),
        "PSXPORT_PAD_REPLAY": a.replay,
    })
    for kv in a.env:
        k, _, v = kv.partition("=")
        env[k] = v

    log = open("scratch/logs/drive_to_grab.log", "w")
    proc = subprocess.Popen([BIN], env=env, stdout=log, stderr=subprocess.STDOUT)
    print("[drive] launched pid=%d port=%d  env: %s" % (proc.pid, a.port, " ".join(a.env) or "(default)"))
    try:
        t0 = time.time()
        held = 0
        while time.time() - t0 < a.timeout:
            if proc.poll() is not None:
                print("[drive] process exited early (rc=%s) — see scratch/logs/drive_to_grab.log" % proc.returncode)
                return 2
            v = word(a.port, ATTACH_PTR)
            if v:
                held = v
                break
            time.sleep(0.5)
        if not held:
            print("[drive] NO GRAB observed within %.0fs — attach pointer G+0x158 stayed 0." % a.timeout)
            return 2

        print("[drive] GRAB observed: Tomba is holding %08X" % held)
        print("        %-6s %-8s %-8s %-8s" % ("t", "+0x2b", "+0x48", "tilt"))
        tilt_ptr = word(a.port, held + 0xC4)
        seen = []
        for i in range(a.samples):
            stamp = byte_at(a.port, held + 0x2B)
            weight = word(a.port, held + 0x48)
            tilt = word(a.port, tilt_ptr + 8) if tilt_ptr else None
            tv = (tilt & 0xFFF) if tilt is not None else None
            seen.append(tv)
            print("        %-6d %-8s %-8s %-8s" % (
                i,
                "?" if stamp is None else "0x%02X" % stamp,
                "?" if weight is None else "0x%04X" % (weight & 0xFFFF),
                "?" if tv is None else "0x%03X" % tv))
            time.sleep(0.4)

        moved = len({x for x in seen if x is not None}) > 1
        print("[drive] VERDICT: tilt %s while grabbed  (%d distinct angles)" %
              ("MOVES" if moved else "is STATIC", len({x for x in seen if x is not None})))
        return 0 if moved else 1
    finally:
        try:
            proc.kill()
        except OSError:
            pass
        log.close()


if __name__ == "__main__":
    sys.exit(main())
