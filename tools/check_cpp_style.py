#!/usr/bin/env python3
"""Enforce formatting and shrink-only size caps on adopted C++ ownership seams."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FILE_CAPS = {
    "game/core/engine.cpp": 4404,
    "game/core/engine.h": 661,
    "game/core/game_hooks.cpp": 388,
    "game/scene/demo.cpp": 1280,
    "game/scene/demo.h": 87,
    "game/scene/sop.cpp": 962,
}


def main() -> int:
    formatter = shutil.which("clang-format")
    if formatter is None:
        print("cpp-style: REFUSED — clang-format is not installed", file=sys.stderr)
        return 2

    paths = [ROOT / relative for relative in FILE_CAPS]
    formatted = subprocess.run(
        [formatter, "--dry-run", "--Werror", *map(str, paths)], cwd=ROOT, check=False
    )
    if formatted.returncode:
        print("cpp-style: FAIL — run clang-format with the repository .clang-format", file=sys.stderr)
        return 1

    failed = False
    for relative, cap in FILE_CAPS.items():
        lines = len((ROOT / relative).read_text(encoding="utf-8").splitlines())
        print(f"cpp-style: {relative}: {lines}/{cap} lines")
        if lines > cap:
            print(
                f"cpp-style: FAIL — {relative} grew past its ownership cap; extract a cohesive module",
                file=sys.stderr,
            )
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
