#!/usr/bin/env python3
"""Positive and negative tests for the stable Tomba! 2 launcher."""

from __future__ import annotations

import importlib.util
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "tools/run.py"
SCRATCH = SCRIPT.parents[1] / "scratch/selftests"
SPEC = importlib.util.spec_from_file_location("tomba_run", SCRIPT)
assert SPEC and SPEC.loader
run = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(run)


def temporary_directory() -> tempfile.TemporaryDirectory[str]:
    SCRATCH.mkdir(parents=True, exist_ok=True)
    return tempfile.TemporaryDirectory(prefix="launcher-", dir=SCRATCH)


class RunLauncherTest(unittest.TestCase):
    def test_resume_parser_preserves_following_disc(self) -> None:
        resume, remaining = run.parse_arguments(["--resume", "route.pad", "game.chd"])
        self.assertEqual(resume, "route.pad")
        self.assertEqual(remaining, ["game.chd"])

    def test_default_resume_snapshots_live_recording(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            live = root / "scratch/bin/pad_session.pad"
            live.parent.mkdir(parents=True)
            live.write_bytes(b"\x01\x02\x03\x04")
            env: dict[str, str] = {}
            run.resolve_resume("", root, env)
            self.assertEqual(
                (root / "scratch/bin/pad_resume.pad").read_bytes(), live.read_bytes()
            )
            self.assertEqual(env["PSXPORT_PAD_RESUME"], "scratch/bin/pad_resume.pad")

    def test_disc_resolution_priority(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            for name in ("cli.chd", "env.chd", "file.chd", "drop.chd"):
                (root / name).touch()
            (root / ".env").write_text(
                "PSXPORT_TOMBA2_DISC=file.chd\n", encoding="utf-8"
            )
            env = {"PSXPORT_TOMBA2_DISC": "env.chd"}
            self.assertEqual(run.resolve_disc("cli.chd", root, env), "cli.chd")
            self.assertEqual(run.resolve_disc(None, root, env), "env.chd")
            self.assertEqual(run.resolve_disc(None, root, {}), "file.chd")

    def test_missing_disc_refuses(self) -> None:
        with (
            temporary_directory() as temp,
            self.assertRaisesRegex(run.LauncherError, "no disc image"),
        ):
            run.resolve_disc(None, Path(temp), {})

    def test_non_clang_toolchain_refuses(self) -> None:
        with (
            mock.patch.object(run, "command_output", return_value="gcc 16"),
            self.assertRaisesRegex(run.LauncherError, "CC=gcc is not Clang"),
        ):
            run.validate_toolchain("gcc", "g++", Path.cwd())

    def test_full_flow_builds_project_target_and_execs_it(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            (root / "external/psxport/cmake").mkdir(parents=True)
            (root / "external/psxport/cmake/psxport.cmake").touch()
            discdump = root / "external/psxport/build/tools/discdump"
            discdump.parent.mkdir(parents=True)
            discdump.touch(mode=0o755)
            main_exe = root / "scratch/bin/tomba2/MAIN.EXE"
            main_exe.parent.mkdir(parents=True)
            main_exe.touch()
            (root / "game.chd").touch()

            commands: list[list[str]] = []
            launched: list[object] = []

            def fake_checked(command: list[str], **_: object) -> None:
                commands.append(list(command))

            def fake_exec(
                program: str, argv: list[str], env: dict[str, str], _: Path
            ) -> None:
                launched.extend([program, argv, env])
                raise run.LauncherError("exec intercepted")

            def fake_output(command: list[str], **_: object) -> str:
                return "clang version 20" if "--version" in command else ""

            with (
                mock.patch.object(run, "require_tool"),
                mock.patch.object(run, "run_checked", side_effect=fake_checked),
                mock.patch.object(run, "command_output", side_effect=fake_output),
                mock.patch.object(run, "exec_program", side_effect=fake_exec),
                mock.patch.object(run, "processor_count", return_value=8),
                mock.patch.dict(os.environ, {}, clear=True),
            ):
                self.assertEqual(run.main(["game.chd"], root=root), 1)

            self.assertIn(
                ["cmake", "--build", "build", "-j", "8", "--target", "tomba2_port"],
                commands,
            )
            self.assertEqual(launched[0], "./scratch/bin/tomba2_port")
            self.assertEqual(
                launched[1],
                ["./scratch/bin/tomba2_port", "scratch/bin/tomba2/MAIN.EXE"],
            )
            self.assertEqual(launched[2]["PSXPORT_ASSET_DIR"], "external/psxport")
            self.assertEqual(launched[2]["PSXPORT_TOMBA2_DISC"], "game.chd")


if __name__ == "__main__":
    unittest.main()
