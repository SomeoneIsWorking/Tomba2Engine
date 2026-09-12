#!/usr/bin/env python3
"""Run the local resident Lightrec discriminator on authenticated MAIN.EXE."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
from collections.abc import Sequence

from tomba2_provision import DEFAULT_OUTPUT, IMAGES, ROOT, ProvisionError, validate_image

DEFAULT_BUILD = ROOT / "build/resident-dispatch"


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=pathlib.Path, default=DEFAULT_BUILD)
    parser.add_argument("--executable-dir", type=pathlib.Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args(arguments)

    executable = args.build_dir / "test_authentic_resident_override"
    if not executable.is_file():
        print(f"REFUSED: focused test executable is missing: {executable}", file=sys.stderr)
        return 2

    identity = IMAGES.get("MAIN.EXE")
    if identity is None:
        print("REFUSED: manifest lacks MAIN.EXE", file=sys.stderr)
        return 2
    path = args.executable_dir / "MAIN.EXE"
    try:
        validate_image(path, "MAIN.EXE", identity)
    except ProvisionError as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2

    return subprocess.run([str(executable), str(path), identity.sha256], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
