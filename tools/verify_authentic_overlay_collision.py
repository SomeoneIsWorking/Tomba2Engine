#!/usr/bin/env python3
"""Run the local A03/A0B Lightrec discriminator on authenticated user overlays."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
from collections.abc import Sequence

from tomba2_provision import (
    DEFAULT_OVERLAYS,
    IMAGES,
    ROOT,
    ProvisionError,
    validate_image,
)

OVERLAY_NAMES = ("A03.BIN", "A0B.BIN")
DEFAULT_BUILD = ROOT / "build/overlay-collision"


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=pathlib.Path, default=DEFAULT_BUILD)
    parser.add_argument("--overlay-dir", type=pathlib.Path, default=DEFAULT_OVERLAYS)
    args = parser.parse_args(arguments)

    executable = args.build_dir / "test_authentic_overlay_collision"
    if not executable.is_file():
        print(
            f"REFUSED: focused test executable is missing: {executable}",
            file=sys.stderr,
        )
        return 2

    inputs: list[str] = []
    for name in OVERLAY_NAMES:
        disc_path = f"BIN/{name}"
        identity = IMAGES.get(disc_path)
        if identity is None:
            print(f"REFUSED: manifest lacks {disc_path}", file=sys.stderr)
            return 2
        path = args.overlay_dir / name
        try:
            validate_image(path, disc_path, identity)
        except ProvisionError as exc:
            print(f"REFUSED: {exc}", file=sys.stderr)
            return 2
        inputs.extend((str(path), identity.sha256))

    return subprocess.run([str(executable), *inputs], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
