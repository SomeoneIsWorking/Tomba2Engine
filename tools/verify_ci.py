#!/usr/bin/env python3
"""Run the complete asset-free Linux native/dynarec repository gate."""

from __future__ import annotations

import argparse
import sys
from collections.abc import Sequence
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD = ROOT / "build/ci"
PSXPORT = (ROOT / "external/psxport").resolve()
sys.path.insert(0, str(PSXPORT / "tools"))

from port.consumer_verify import ConsumerVerifyConfig, run_consumer_verification


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=DEFAULT_BUILD)
    args = parser.parse_args(arguments)
    build = args.build.resolve()
    return run_consumer_verification(
        ConsumerVerifyConfig(
            name="Tomba native/Lightrec products",
            root=ROOT,
            build=build,
            psxport=PSXPORT,
            product=build / "bin/tomba2_port",
            cmake_module=ROOT / "cmake/tomba2_port.cmake",
            test_regex=".",
            cmake_definitions=(
                "-DBUILD_TESTING=ON",
                "-DPSXPORT_BUILD_PORT=ON",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ),
            python=Path(sys.executable),
        )
    )


if __name__ == "__main__":
    raise SystemExit(main())
