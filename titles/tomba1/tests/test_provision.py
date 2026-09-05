"""Hermetic checks for the shipping Tomba! 1 disc provisioner."""

from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest

TITLE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TITLE_ROOT / "tools"))

import provision
import verify_executable


class FakeExtractor:
    def __init__(self, files: dict[str, bytes]) -> None:
        self.files = files
        self.requests: list[str] = []

    def __call__(
        self, disc_path: str, _disc: pathlib.Path, destination: pathlib.Path
    ) -> pathlib.Path:
        self.requests.append(disc_path)
        if disc_path not in self.files:
            raise provision.Refused(f"fixture has no {disc_path}")
        destination.mkdir(parents=True, exist_ok=True)
        output = destination / pathlib.PurePosixPath(disc_path).name
        output.write_bytes(self.files[disc_path])
        return output


class ProvisionTests(unittest.TestCase):
    def test_resolution_precedence_is_cli_env_envfile_drop(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            paths = {
                name: root / name
                for name in ("cli.chd", "env.chd", "file.chd", "drop.chd")
            }
            for path in paths.values():
                path.write_bytes(b"disc")
            (root / ".env").write_text(
                f"{provision.DISC_ENV}={paths['file.chd'].name}\n", encoding="utf-8"
            )

            selected = provision.resolve_disc(
                paths["cli.chd"].name,
                root,
                {provision.DISC_ENV: paths["env.chd"].name},
            )
            self.assertEqual(selected.path, paths["cli.chd"])
            selected = provision.resolve_disc(
                None, root, {provision.DISC_ENV: paths["env.chd"].name}
            )
            self.assertEqual(selected.path, paths["env.chd"])
            selected = provision.resolve_disc(None, root, {})
            self.assertEqual(selected.path, paths["file.chd"])

            (root / ".env").unlink()
            paths["cli.chd"].unlink()
            paths["env.chd"].unlink()
            paths["file.chd"].unlink()
            selected = provision.resolve_disc(None, root, {})
            self.assertEqual(selected.path, paths["drop.chd"])

    def test_multiple_drop_in_chds_refuse_ambiguity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            (root / "a.chd").write_bytes(b"a")
            (root / "b.CHD").write_bytes(b"b")
            with self.assertRaisesRegex(
                provision.Refused, "2 repository-root CHDs are ambiguous"
            ):
                provision.resolve_disc(None, root, {})

    def test_missing_explicit_disc_refuses_without_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            (root / "fallback.chd").write_bytes(b"fallback")
            with self.assertRaisesRegex(
                provision.Refused, "command line selects missing"
            ):
                provision.resolve_disc("missing.chd", root, {})

    def test_system_cnf_normalizes_the_retail_boot_syntax(self) -> None:
        boot = provision.normalize_boot_path(
            b"BOOT = cdrom:\\SCUS_942.36;1\r\nTCB = 4\r\n"
        )
        self.assertEqual(boot, "SCUS_942.36")

    def test_system_cnf_requires_one_cdrom_boot(self) -> None:
        with self.assertRaisesRegex(provision.Refused, "exactly one"):
            provision.normalize_boot_path(b"TCB = 4\nEVENT = 10\n")
        with self.assertRaisesRegex(provision.Refused, "found 2"):
            provision.normalize_boot_path(
                b"BOOT=cdrom:\\SCUS_942.36;1\nBOOT=cdrom:\\OTHER.EXE;1\n"
            )

    def test_provision_publishes_only_after_boot_and_identity_agree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            image = verify_executable.fixture_image()
            manifest = verify_executable.fixture_manifest(image)
            manifest["disc_executable"] = "SCUS_TEST.00"
            output = root / "published/SCUS_TEST.00"
            extractor = FakeExtractor(
                {
                    provision.SYSTEM_CNF: b"BOOT=cdrom:\\SCUS_TEST.00;1\n",
                    "SCUS_TEST.00": image,
                }
            )

            result = provision.provision(
                root / "disc.chd", output, extractor, manifest, root / "staging"
            )

            self.assertEqual(extractor.requests, [provision.SYSTEM_CNF, "SCUS_TEST.00"])
            self.assertEqual(result.identity_checks, 15)
            self.assertEqual(output.read_bytes(), image)

    def test_wrong_boot_target_is_mismatch_and_preserves_existing_output(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            image = verify_executable.fixture_image()
            manifest = verify_executable.fixture_manifest(image)
            manifest["disc_executable"] = "SCUS_TEST.00"
            output = root / "SCUS_TEST.00"
            output.write_bytes(b"existing")
            extractor = FakeExtractor(
                {provision.SYSTEM_CNF: b"BOOT=cdrom:\\SCUS_WRONG.00;1\n"}
            )

            with self.assertRaisesRegex(provision.Mismatch, "SCUS_WRONG.00"):
                provision.provision(
                    root / "disc.chd", output, extractor, manifest, root / "staging"
                )
            self.assertEqual(output.read_bytes(), b"existing")

    def test_identity_mismatch_never_publishes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            image = verify_executable.fixture_image()
            manifest = verify_executable.fixture_manifest(image)
            manifest["disc_executable"] = "SCUS_TEST.00"
            altered = bytearray(image)
            altered[-1] ^= 0xFF
            output = root / "SCUS_TEST.00"
            extractor = FakeExtractor(
                {
                    provision.SYSTEM_CNF: b"BOOT=cdrom:\\SCUS_TEST.00;1\n",
                    "SCUS_TEST.00": bytes(altered),
                }
            )

            with self.assertRaisesRegex(provision.Mismatch, "SHA-1"):
                provision.provision(
                    root / "disc.chd", output, extractor, manifest, root / "staging"
                )
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
