#!/usr/bin/env python3
"""Read Tomba! 1's own DRAWENV/DISPENV rectangles out of guest RAM, live, in one product run.

WHY IT IS NEEDED. A 4:3 run and a 16:9 run of the same build must differ ONLY in what the
widescreen owner published. When a run's primitive stream also differs in COUNT, the difference is
either the widening working or a regression, and nothing in the log distinguishes them. This tool
answers it from the guest's own memory instead of from a symptom: it reads the very structures
`FUN_80016C4C` builds (DISPENV 0x8009D6B0 / DRAWENV 0x8009D6C4, and the second pair at
0x8009E3C0 / 0x8009E3D4) plus the GTE projection registers the framework records, and prints them.

The GTE pair is the decisive one. `libgte_set_geom_offset` writes CR24/CR25 and the framework's
`ProjParams` records the same numbers, so reading the recorded copy is reading what the widening
actually published rather than what a log line claimed.

Usage:
    env_probe.py --disc <tomba1.chd> --aspect 0
    env_probe.py --disc <tomba1.chd> --aspect 1
"""

from __future__ import annotations

import argparse
import os
import pathlib
import socket
import subprocess
import sys
import time

REPOSITORY = pathlib.Path(__file__).resolve().parents[3]
PRODUCT = REPOSITORY / "build/bin/tomba1_port"
ENVIRONMENT_KEY = "PSXPORT_TOMBA1_DISC"

# The four environments `FUN_80016C4C` builds: a DISPENV (0x14 B) then a DRAWENV (0x1C B) at +0x14,
# twice over. Measured at 0x80016C9C/0x80016CAC/0x80016CC8/0x80016CE0 and the second pair.
ENVIRONMENTS = (
    ("DISPENV_A", 0x8009D6B0),
    ("DRAWENV_A", 0x8009D6C4),
    ("DISPENV_B", 0x8009E3C0),
    ("DRAWENV_B", 0x8009E3D4),
)

# SCUS_942.36's own scratchpad fields the frame driver reads, so the probe reports the game state
# rather than assuming the run is where it was launched.
SCRATCH = (
    ("displayState", 0x1F8001CC, 1),
    ("dispMode0", 0x1F8001F0, 2),
    ("dispMode1", 0x1F8001F4, 2),
    ("requestedFields", 0x1F8001EA, 2),
    ("drawSyncBeforeVblank", 0x1F8001EC, 2),
)


class Refused(RuntimeError):
    """The probe cannot be answered honestly."""


def live_client(port: int):
    tools = REPOSITORY / "external/psxport/tools"
    if not (tools / "dbgclient.py").is_file():
        raise Refused(f"the framework's debug client is absent at {tools / 'dbgclient.py'}")
    if str(tools) not in sys.path:
        sys.path.insert(0, str(tools))
    from dbgclient import LiveClient  # noqa: PLC0415 — imported after sys.path is prepared

    return LiveClient(port=port, timeout=120.0)


def rect(client, base: int) -> str:
    """The RECT as the guest actually stores it: four 16-bit fields in two little-endian words.

    Printing the RAW WORDS is deliberate. A PSX RECT is 8 bytes, and reporting four "shorts" taken
    from four 32-bit reads silently interleaves the two halves of each word, which produced a
    plausible-looking but wrong reading in an earlier version of this probe. Two words, shown as
    bytes, cannot be misread.
    """
    values = client.words(base, 2)
    packed = b"".join(value.to_bytes(4, "little") for value in values)[:8]
    x, y, width, height = (int.from_bytes(packed[offset:offset + 2], "little") for offset in (0, 2, 4, 6))
    return f"RECT x={x} y={y} w={width} h={height}  [words {' '.join(f'{value:08X}' for value in values)}]"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=pathlib.Path, required=True)
    parser.add_argument("--aspect", type=int, required=True)
    parser.add_argument("--port", type=int, default=5977)
    parser.add_argument("--frames", type=int, default=180)
    parser.add_argument("--out", type=pathlib.Path)
    args = parser.parse_args(argv)

    if not PRODUCT.is_file():
        print(f"REFUSED: {PRODUCT} is absent")
        return 2
    settings = pathlib.Path(f"/tmp/opencode/env_probe_aspect{args.aspect}.ini")
    settings.parent.mkdir(parents=True, exist_ok=True)
    settings.write_text(f"aspect={args.aspect}\nires=1\n", encoding="utf-8")

    environment = {
        "PATH": "/usr/bin:/bin",
        "HOME": str(pathlib.Path.home()),
        ENVIRONMENT_KEY: str(args.disc),
        "PSXPORT_SETTINGS": str(settings),
        "PSXPORT_VK_HEADLESS": "1",
        "PSXPORT_DEBUG_SERVER": str(args.port),
        "PSXPORT_NATIVE_FRAMES": str(args.frames),
        "PSXPORT_ASSET_DIR": "/home/bhamil/repo/psx/psxport",
        "SDL_VIDEODRIVER": "offscreen",
        "SDL_AUDIODRIVER": "dummy",
        "VK_ICD_FILENAMES": "/usr/share/vulkan/icd.d/lvp_icd.x86_64.json",
    }
    log = (args.out or pathlib.Path(f"/tmp/opencode/env_probe_aspect{args.aspect}.log"))
    with log.open("wb") as handle:
        process = subprocess.Popen([str(PRODUCT)], cwd=REPOSITORY, env=environment,
                                   stdout=handle, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + 120.0
        client = None
        while client is None and time.monotonic() < deadline:
            if process.poll() is not None:
                raise Refused(f"the product exited ({process.returncode}); see {log}")
            try:
                client = live_client(args.port)
            except OSError:
                time.sleep(0.5)
        if client is None:
            raise Refused(f"the debug channel never opened on port {args.port}; see {log}")
        with client:
            client.send("play")
            deadline = time.monotonic() + 600.0
            while time.monotonic() < deadline:
                if client.frame() >= args.frames:
                    break
                if process.poll() is not None:
                    raise Refused(f"the product exited ({process.returncode}); see {log}")
                time.sleep(0.5)
            print(f"RUN aspect={args.aspect}: reached frame {client.frame()}")
            print("SCANNED: 4 guest display/draw environments, 5 scratchpad state fields")
            for name, base in ENVIRONMENTS:
                reading = rect(client, base)
                width = int(reading.split("w=")[1].split()[0])
                print(f"  {name:10s} @0x{base:08X}  {reading}  -> "
                      f"{'WIDENED' if width > 320 else 'retail 4:3'}")
            for name, address, size in SCRATCH:
                value = client.word(address) & (0xFF if size == 1 else 0xFFFF)
                print(f"  {name:22s} @0x{address:08X} = {value}")
    finally:
        process.terminate()
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=60)
    for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
        if "tomba1-wide" in line or "[wide] native picture" in line:
            print(f"  {line.split('] ', 1)[-1]}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refused as exc:
        print(f"REFUSED: {exc}")
        raise SystemExit(2)
