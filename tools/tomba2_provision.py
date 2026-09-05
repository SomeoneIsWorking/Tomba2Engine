#!/usr/bin/env python3
"""Extract the exact Tomba! 2 runtime images from a user-supplied disc."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
from collections.abc import Callable, Mapping, Sequence

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "scratch/bin/tomba2"
DEFAULT_OVERLAYS = ROOT / "scratch/bin/overlays"

IMAGE_SIZES: Mapping[str, int] = {
    "MAIN.EXE": 716_800,
    "SCUS_944.54": 167_936,
    "BIN/START.BIN": 1_648,
    "BIN/DEMO.BIN": 5_372,
    "BIN/GAME.BIN": 11_636,
    "BIN/SOP.BIN": 17_660,
    "BIN/OPN.BIN": 13_596,
    "BIN/CRD.BIN": 25_060,
    "BIN/A00.BIN": 285_096,
    "BIN/A01.BIN": 202_460,
    "BIN/A02.BIN": 201_004,
    "BIN/A03.BIN": 78_444,
    "BIN/A04.BIN": 250_788,
    "BIN/A05.BIN": 234_164,
    "BIN/A06.BIN": 284_348,
    "BIN/A07.BIN": 229_720,
    "BIN/A08.BIN": 266_124,
    "BIN/A09.BIN": 22_125,
    "BIN/A0A.BIN": 126_520,
    "BIN/A0B.BIN": 115_560,
    "BIN/A0C.BIN": 123_328,
    "BIN/A0D.BIN": 119_592,
    "BIN/A0E.BIN": 124_656,
    "BIN/A0F.BIN": 135_500,
    "BIN/A0G.BIN": 17_980,
    "BIN/A0H.BIN": 14_016,
    "BIN/A0I.BIN": 15_152,
    "BIN/A0J.BIN": 19_068,
    "BIN/A0K.BIN": 93_380,
    "BIN/A0L.BIN": 74_832,
}


class ProvisionError(RuntimeError):
    """The selected disc could not supply the complete target image set."""


def destination_for(
    disc_path: str, executable_root: pathlib.Path, overlay_root: pathlib.Path
) -> pathlib.Path:
    name = pathlib.PurePosixPath(disc_path).name
    return overlay_root / name if disc_path.startswith("BIN/") else executable_root / name


def validate_image(path: pathlib.Path, disc_path: str, expected_size: int) -> None:
    try:
        actual_size = path.stat().st_size
    except OSError as exc:
        raise ProvisionError(f"cannot inspect extracted {disc_path}: {exc}") from exc
    if actual_size != expected_size:
        raise ProvisionError(
            f"{disc_path} is {actual_size} bytes; expected {expected_size} for the selected title"
        )


def extract_image(
    discdump: pathlib.Path,
    disc: pathlib.Path,
    disc_path: str,
    destination: pathlib.Path,
) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(
        [str(discdump), "get", disc_path, str(disc), str(destination.parent)],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    if completed.returncode != 0 or not destination.is_file():
        diagnostic = completed.stderr.decode(errors="replace").strip()
        suffix = f": {diagnostic}" if diagnostic else ""
        raise ProvisionError(f"discdump could not extract {disc_path}{suffix}")


def provision(
    discdump: pathlib.Path,
    disc: pathlib.Path,
    executable_root: pathlib.Path = DEFAULT_OUTPUT,
    overlay_root: pathlib.Path = DEFAULT_OVERLAYS,
    *,
    images: Mapping[str, int] = IMAGE_SIZES,
    extractor: Callable[[pathlib.Path, pathlib.Path, str, pathlib.Path], None] = extract_image,
) -> tuple[pathlib.Path, ...]:
    if not disc.is_file():
        raise ProvisionError(f"disc image does not exist: {disc}")
    if not discdump.is_file():
        raise ProvisionError(f"discdump executable does not exist: {discdump}")

    outputs: list[pathlib.Path] = []
    for disc_path, expected_size in images.items():
        destination = destination_for(disc_path, executable_root, overlay_root)
        if not destination.is_file():
            extractor(discdump, disc, disc_path, destination)
        validate_image(destination, disc_path, expected_size)
        outputs.append(destination)
    return tuple(outputs)


def parse_args(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disc", type=pathlib.Path)
    parser.add_argument("--discdump", required=True, type=pathlib.Path)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_args(arguments)
    try:
        outputs = provision(args.discdump, args.disc)
    except ProvisionError as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 1
    print(f"PROVISIONED: {len(outputs)}/{len(IMAGE_SIZES)} runtime images")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
