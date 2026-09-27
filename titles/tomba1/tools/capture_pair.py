#!/usr/bin/env python3
"""Capture a matched Tomba! 1 4:3 / wide present pair through the product's own debug channel.

WHY IT IS A SEPARATE OWNER AND NOT A FLAG. A widescreen claim is a claim about a RELATIONSHIP
between two pictures of the same game state, and that relationship is the only thing
`tools/port/widescreen_pair.py` can judge. This tool therefore runs the product TWICE from the same
checkout, drives both to the SAME determinate checkpoint over the framework's own loopback debug
channel, captures what is PRESENTED in each, and refuses to produce a pair whose checkpoints differ.

It drives the shipped binary and nothing else: no reimplementation of the renderer, no frame
comparison, and no synthetic image. The two runs differ in exactly one input, the aspect setting, and
the tool reports the `[wide]` announcement line from each so the reader can see which leg resolved to
which width rather than being asked to trust the pairing.

Refusals, each a real failure rather than a soft result:
  * another product instance is already running — two game instances on this machine are forbidden;
  * the product does not reach the requested checkpoint;
  * the two runs do not report the SAME frame, since a pair from two game states measures nothing;
  * either capture is missing, is not a PPM, or is a single flat colour (a uniform frame makes every
    widening hypothesis score identically and must not be reported as a pair).

Usage:
    capture_pair.py --disc <tomba1.chd> --out titles/tomba1/scratch/wide
    capture_pair.py --disc <tomba1.chd> --out ... --checkpoint 420 --port 5971
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
import time

REPOSITORY = pathlib.Path(__file__).resolve().parents[3]
PRODUCT = REPOSITORY / "build/bin/tomba1_port"
ENVIRONMENT_KEY = "PSXPORT_TOMBA1_DISC"

# The two legs. ASPECT_16_9 is the widening the title declares; ASPECT_4_3 is the retail reference.
LEGS = (("narrow", 0), ("wide", 1))

# A process name that means a product instance is already running on this machine. Checked BEFORE
# launching, because starting a second one to find out is the thing the contract forbids.
PRODUCT_PROCESS_NAMES = ("tomba1_port", "tomba2_port")

WIDE_ANNOUNCE = re.compile(r"\[wide\] native picture: .*")


class Refused(RuntimeError):
    """The requested capture cannot be produced honestly."""


def sys_path_entry() -> str:
    """The framework tools directory, so the protocol client is the framework's own implementation."""
    tools = REPOSITORY / "external/psxport/tools"
    if not (tools / "dbgclient.py").is_file():
        raise Refused(f"the framework's debug client is absent at {tools / 'dbgclient.py'}")
    return str(tools)


def live_client(port: int):
    """The framework's own debug client, imported rather than reimplemented."""
    path = sys_path_entry()
    if path not in sys.path:
        sys.path.insert(0, path)
    from dbgclient import LiveClient  # noqa: PLC0415 — imported after sys.path is prepared, on purpose

    return LiveClient(port=port, timeout=120.0)


def existing_product_instances() -> list[str]:
    listing = subprocess.run(["ps", "-eo", "comm"], capture_output=True, text=True, check=False)
    return sorted({line.strip() for line in listing.stdout.splitlines()
                   if line.strip() in PRODUCT_PROCESS_NAMES})


def read_ppm(path: pathlib.Path) -> tuple[int, int, int, int]:
    """(width, height, distinct_colour_count, widest_run) of a binary P6 PPM.

    `distinct_colour_count` and `widest_run` are the uniformity tell the discriminator names: a frame
    that is one flat colour, or one that is a few colours smeared across a whole row, makes every
    hypothesis score the same and cannot be evidence of anything.
    """
    raw = path.read_bytes()
    fields: list[bytes] = []
    index = 0
    while len(fields) < 4:
        while index < len(raw) and raw[index:index + 1].isspace():
            index += 1
        if raw[index:index + 1] == b"#":
            while index < len(raw) and raw[index:index + 1] != b"\n":
                index += 1
            continue
        start = index
        while index < len(raw) and not raw[index:index + 1].isspace():
            index += 1
        fields.append(raw[start:index])
    if fields[0] != b"P6":
        raise Refused(f"{path} is not a binary PPM (magic {fields[0]!r})")
    width, height = int(fields[1]), int(fields[2])
    pixels = raw[index + 1:index + 1 + width * height * 3]
    colours = {pixels[offset:offset + 3] for offset in range(0, len(pixels) - 2, 3)}
    widest_run = 0
    if width and len(pixels) >= width * 3:
        row = 0
        first = pixels[0:3]
        for column in range(width):
            pixel = pixels[column * 3:column * 3 + 3]
            if pixel == first:
                row += 1
                widest_run = max(widest_run, row)
            else:
                row = 1
                first = pixel
    return width, height, len(colours), widest_run


def run_leg(disc: pathlib.Path, settings: pathlib.Path, out_dir: pathlib.Path, checkpoint: int,
            port: int) -> tuple[pathlib.Path, int, str]:
    if not PRODUCT.is_file():
        raise Refused(f"{PRODUCT} is absent; build the tomba1_port target first")

    environment = {
        "PATH": "/usr/bin:/bin",
        "HOME": str(pathlib.Path.home()),
        ENVIRONMENT_KEY: str(disc),
        "PSXPORT_SETTINGS": str(settings),
        "PSXPORT_VK_HEADLESS": "1",
        "PSXPORT_DEBUG_SERVER": str(port),
        "SDL_VIDEODRIVER": "offscreen",
        "SDL_AUDIODRIVER": "dummy",
        "VK_ICD_FILENAMES": "/usr/share/vulkan/icd.d/lvp_icd.x86_64.json",
    }
    log_path = out_dir / f"run_{settings.stem}.log"
    with log_path.open("wb") as handle:
        process = subprocess.Popen([str(PRODUCT)], cwd=REPOSITORY, env=environment,
                                   stdout=handle, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + 120.0
        client = None
        while client is None and time.monotonic() < deadline:
            if process.poll() is not None:
                raise Refused(f"the product exited ({process.returncode}) before its channel opened; "
                              f"see {log_path}")
            try:
                client = live_client(port)
            except OSError:
                time.sleep(0.5)
        if client is None:
            raise Refused(f"the product's debug channel never opened on port {port}; see {log_path}")

        with client:
            # The product runs unpaced; drive it by asking for the frame counter until the
            # checkpoint passes, so both legs stop on the SAME presented frame by construction.
            drive_deadline = time.monotonic() + 900.0
            frame = 0
            while time.monotonic() < drive_deadline:
                frame = client.frame()
                if frame >= checkpoint:
                    break
                if process.poll() is not None:
                    raise Refused(f"the product exited ({process.returncode}) at frame {frame}; see {log_path}")
                time.sleep(0.5)
            if frame < checkpoint:
                raise Refused(f"the product reached only frame {frame} of {checkpoint} within 900 s; "
                              f"see {log_path}")
            shot = out_dir / f"{settings.stem}_f{frame}.ppm"
            client.shot(str(shot.resolve()))
    finally:
        process.terminate()
        try:
            process.wait(timeout=60)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=60)

    if not shot.is_file() or shot.stat().st_size < 4096:
        raise Refused(f"the capture at {shot} is missing or too small to be a picture; see {log_path}")
    width, height, colours, widest = read_ppm(shot)
    if colours < 16 or widest > width // 2:
        raise Refused(
            f"the capture at {shot} is {width}x{height} with only {colours} distinct colour(s) and a "
            f"{widest}-pixel run: a frame that uniform makes every widening hypothesis score the same, "
            "so it cannot be evidence of a widening or of a stretch"
        )
    announcement = ""
    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if WIDE_ANNOUNCE.search(line):
            announcement = WIDE_ANNOUNCE.search(line).group(0)
    return shot, frame, announcement


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=pathlib.Path, required=True)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    parser.add_argument("--checkpoint", type=int, default=420)
    parser.add_argument("--port", type=int, default=5971)
    args = parser.parse_args(argv)

    running = existing_product_instances()
    if running:
        print(f"REFUSED: a product instance is already running ({', '.join(running)}); two game "
              "instances must never share this machine")
        return 2
    if not args.disc.is_file():
        print(f"REFUSED: {args.disc} is not a file")
        return 2

    out_dir = args.out if args.out.is_absolute() else REPOSITORY / args.out
    out_dir.mkdir(parents=True, exist_ok=True)

    captures: dict[str, pathlib.Path] = {}
    frames: dict[str, int] = {}
    announcements: dict[str, str] = {}
    for name, aspect in LEGS:
        settings = out_dir / f"settings_{name}.ini"
        settings.write_text(f"aspect={aspect}\nires=1\n", encoding="utf-8")
        print(f"LEG {name}: aspect={aspect}, driving to frame {args.checkpoint} on port {args.port}")
        shot, frame, announcement = run_leg(args.disc, settings, out_dir, args.checkpoint, args.port)
        captures[name] = shot
        frames[name] = frame
        announcements[name] = announcement
        print(f"LEG {name}: reached frame {frame}; captured {shot} ({shot.stat().st_size} B)")
        print(f"LEG {name}: {announcement or '(no [wide] announcement in this run)'}")

    if len(set(frames.values())) != 1:
        print(f"REFUSED: the two legs stopped at different frames ({frames}); a pair from two game "
              "states measures nothing")
        return 2

    print()
    print(f"PAIR matched at frame {frames['narrow']}")
    print(f"PAIR narrow={captures['narrow']}")
    print(f"PAIR wide={captures['wide']}")
    print(f"PAIR verdict: uv run --frozen python external/psxport/tools/port/widescreen_pair.py "
          f"--narrow {captures['narrow']} --wide {captures['wide']}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refused as exc:
        print(f"REFUSED: {exc}")
        raise SystemExit(2)
