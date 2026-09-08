#!/usr/bin/env python3
"""Focused tests for authenticated Tomba! 2 runtime-image provisioning."""

from __future__ import annotations

import hashlib
import importlib.util
import io
import tempfile
import sys
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "tools/tomba2_provision.py"
SCRATCH = SCRIPT.parents[1] / "scratch/selftests"
SPEC = importlib.util.spec_from_file_location("tomba2_provision", SCRIPT)
assert SPEC and SPEC.loader
provisioner = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = provisioner
SPEC.loader.exec_module(provisioner)


def temporary_directory() -> tempfile.TemporaryDirectory[str]:
    SCRATCH.mkdir(parents=True, exist_ok=True)
    return tempfile.TemporaryDirectory(prefix="tomba2-provision-", dir=SCRATCH)


def identity(data: bytes) -> provisioner.ImageIdentity:
    return provisioner.ImageIdentity(len(data), hashlib.sha256(data).hexdigest())


class Tomba2ProvisionTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = temporary_directory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.disc = self.root / "disc.chd"
        self.discdump = self.root / "discdump"
        self.disc.touch()
        self.discdump.touch()
        self.executables = self.root / "executables"
        self.overlays = self.root / "overlays"
        self.executables.mkdir()
        self.overlays.mkdir()
        self.images = {"MAIN.EXE": identity(b"main"), "BIN/A03.BIN": identity(b"bin")}
        self.contents = {"MAIN.EXE": b"main", "BIN/A03.BIN": b"bin"}
        self.selected: list[str] = []

    def extract(self, _: Path, __: Path, disc_path: str, destination: Path) -> None:
        self.selected.append(disc_path)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(self.contents[disc_path])

    def provision(self) -> tuple[Path, ...]:
        return provisioner.provision(
            self.discdump, self.disc, self.executables, self.overlays,
            images=self.images, extractor=self.extract,
        )

    def test_provisions_executable_and_overlay_to_distinct_owners(self) -> None:
        self.assertEqual(
            self.provision(),
            (self.executables / "MAIN.EXE", self.overlays / "A03.BIN"),
        )
        self.assertEqual(self.selected, list(self.images))
        self.assertEqual((self.executables / "MAIN.EXE").read_bytes(), b"main")
        self.assertEqual((self.overlays / "A03.BIN").read_bytes(), b"bin")
        self.assertFalse((self.executables / ".provisioning").exists())

    def test_refuses_an_extracted_image_with_the_wrong_size(self) -> None:
        self.contents["MAIN.EXE"] = b"bad"
        with self.assertRaisesRegex(provisioner.ProvisionError, "expected 4"):
            self.provision()

    def test_refuses_oversized_image_after_only_one_bounded_read(self) -> None:
        class RecordingReader(io.BytesIO):
            def __init__(self, data: bytes) -> None:
                super().__init__(data)
                self.read_sizes: list[int] = []

            def read(self, size: int = -1) -> bytes:
                self.read_sizes.append(size)
                return super().read(size)

        source = RecordingReader(b"main" * 16_384)
        # A stale filesystem size must not turn authentication into an unbounded
        # stream hash. The production owner must bound the read itself.
        with (
            mock.patch.object(Path, "stat", return_value=mock.Mock(st_size=4)),
            mock.patch.object(Path, "open", return_value=source),
            self.assertRaisesRegex(provisioner.ProvisionError, "more than 4 bytes; expected 4"),
        ):
            provisioner.validate_image(self.executables / "MAIN.EXE", "MAIN.EXE", identity(b"main"))
        self.assertEqual(source.read_sizes, [5])

    def test_refuses_missing_disc_before_extraction(self) -> None:
        self.disc = self.root / "missing.chd"
        with self.assertRaisesRegex(provisioner.ProvisionError, "disc image does not exist"):
            self.provision()
        self.assertEqual(self.selected, [])

    def test_refuses_same_size_modified_bytes(self) -> None:
        self.contents["MAIN.EXE"] = b"Main"
        with self.assertRaisesRegex(provisioner.ProvisionError, "SHA-256.*does not match"):
            self.provision()

    def test_existing_cache_cannot_hide_a_different_selected_disc(self) -> None:
        cached = self.executables / "MAIN.EXE"
        cached.write_bytes(b"main")
        self.contents["MAIN.EXE"] = b"Main"
        with self.assertRaisesRegex(provisioner.ProvisionError, "SHA-256"):
            self.provision()
        self.assertEqual(self.selected, ["MAIN.EXE"])
        self.assertEqual(cached.read_bytes(), b"main")
        self.assertFalse((self.executables / ".provisioning").exists())

    def test_late_invalid_overlay_preserves_all_existing_outputs(self) -> None:
        # Sentinel bytes establish that no earlier file was published before the
        # later overlay failed; preservation does not depend on identical content.
        (self.executables / "MAIN.EXE").write_bytes(b"previous-main")
        (self.overlays / "A03.BIN").write_bytes(b"previous-overlay")
        self.contents["BIN/A03.BIN"] = b"bad"
        with self.assertRaisesRegex(provisioner.ProvisionError, "A03.BIN.*SHA-256"):
            self.provision()
        self.assertEqual((self.executables / "MAIN.EXE").read_bytes(), b"previous-main")
        self.assertEqual((self.overlays / "A03.BIN").read_bytes(), b"previous-overlay")
        self.assertFalse((self.executables / ".provisioning").exists())

    def test_authenticated_disc_replaces_corrupt_cache(self) -> None:
        (self.executables / "MAIN.EXE").write_bytes(b"Main")
        self.provision()
        self.assertEqual((self.executables / "MAIN.EXE").read_bytes(), b"main")
        self.assertFalse((self.executables / ".provisioning").exists())

    def test_empty_manifest_refuses_instead_of_authenticating_nothing(self) -> None:
        self.images = {}
        with self.assertRaisesRegex(provisioner.ProvisionError, "manifest is empty"):
            self.provision()

    def test_extraction_failure_cleans_staging_and_preserves_cache(self) -> None:
        cached = self.executables / "MAIN.EXE"
        cached.write_bytes(b"main")

        def fail(_: Path, __: Path, ___: str, destination: Path) -> None:
            destination.write_bytes(b"partial")
            raise provisioner.ProvisionError("discdump could not extract MAIN.EXE")

        with self.assertRaisesRegex(provisioner.ProvisionError, "discdump could not extract"):
            provisioner.provision(
                self.discdump, self.disc, self.executables, self.overlays,
                images=self.images, extractor=fail,
            )
        self.assertEqual(cached.read_bytes(), b"main")
        self.assertFalse((self.executables / ".provisioning").exists())


if __name__ == "__main__":
    unittest.main()
