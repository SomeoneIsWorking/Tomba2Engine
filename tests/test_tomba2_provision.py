#!/usr/bin/env python3
"""Focused tests for authenticated Tomba! 2 runtime-image provisioning."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools/tomba2_provision.py"
SCRATCH = SCRIPT.parents[1] / "scratch/selftests"
SPEC = importlib.util.spec_from_file_location("tomba2_provision", SCRIPT)
assert SPEC and SPEC.loader
provisioner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(provisioner)


def temporary_directory() -> tempfile.TemporaryDirectory[str]:
    SCRATCH.mkdir(parents=True, exist_ok=True)
    return tempfile.TemporaryDirectory(prefix="tomba2-provision-", dir=SCRATCH)


class Tomba2ProvisionTest(unittest.TestCase):
    def test_provisions_executable_and_overlay_to_distinct_owners(self) -> None:
        images = {"MAIN.EXE": 4, "BIN/A00.BIN": 3}
        with temporary_directory() as temp:
            root = Path(temp)
            disc = root / "disc.chd"
            discdump = root / "discdump"
            disc.touch()
            discdump.touch()

            def extract(
                _: Path, __: Path, disc_path: str, destination: Path
            ) -> None:
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(bytes(images[disc_path]))

            outputs = provisioner.provision(
                discdump,
                disc,
                root / "executables",
                root / "overlays",
                images=images,
                extractor=extract,
            )

            self.assertEqual(
                outputs,
                (root / "executables/MAIN.EXE", root / "overlays/A00.BIN"),
            )

    def test_refuses_an_extracted_image_with_the_wrong_size(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            disc = root / "disc.chd"
            discdump = root / "discdump"
            disc.touch()
            discdump.touch()

            def extract(_: Path, __: Path, ___: str, destination: Path) -> None:
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(b"bad")

            with self.assertRaisesRegex(provisioner.ProvisionError, "expected 4"):
                provisioner.provision(
                    discdump,
                    disc,
                    root / "executables",
                    root / "overlays",
                    images={"MAIN.EXE": 4},
                    extractor=extract,
                )

    def test_refuses_missing_disc_before_extraction(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            discdump = root / "discdump"
            discdump.touch()
            with self.assertRaisesRegex(provisioner.ProvisionError, "disc image does not exist"):
                provisioner.provision(discdump, root / "missing.chd", images={})


if __name__ == "__main__":
    unittest.main()
