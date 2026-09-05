#!/usr/bin/env python3
"""Positive and negative tests for the stable Tomba! 2 launcher."""

from __future__ import annotations

import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[1] / "tools/run.py"
REPO = SCRIPT.parents[1]
LAUNCHER_SCRIPT = REPO / "tools/launcher.py"
SCRATCH = SCRIPT.parents[1] / "scratch/selftests"
SPEC = importlib.util.spec_from_file_location("tomba_run", SCRIPT)
assert SPEC and SPEC.loader
run = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(run)
LAUNCHER_SPEC = importlib.util.spec_from_file_location(
    "tomba_launcher", LAUNCHER_SCRIPT
)
assert LAUNCHER_SPEC and LAUNCHER_SPEC.loader
launcher = importlib.util.module_from_spec(LAUNCHER_SPEC)
LAUNCHER_SPEC.loader.exec_module(launcher)


def temporary_directory() -> tempfile.TemporaryDirectory[str]:
    SCRATCH.mkdir(parents=True, exist_ok=True)
    return tempfile.TemporaryDirectory(prefix="launcher-", dir=SCRATCH)


class RunLauncherTest(unittest.TestCase):
    def test_product_selector_preserves_legacy_tomba2_disc_argument(self) -> None:
        self.assertEqual(launcher.select_product([]), ("tomba2", []))
        self.assertEqual(
            launcher.select_product(["legacy-disc.chd"]),
            ("tomba2", ["legacy-disc.chd"]),
        )
        self.assertEqual(
            launcher.select_product(["tomba1", "first-disc.chd"]),
            ("tomba1", ["first-disc.chd"]),
        )
        self.assertEqual(
            launcher.select_product(["tomba2", "second-disc.chd"]),
            ("tomba2", ["second-disc.chd"]),
        )

    def test_top_level_and_selected_product_help_precede_discovery(self) -> None:
        environment = {"PATH": "", "PYTHONPATH": ""}
        cases = (
            (["-h"], "Products:"),
            (["--help"], "Products:"),
            (["tomba1", "-h"], "Tomba! USA"),
            (["tomba1", "--help"], "Tomba! USA"),
            (["tomba2", "-h"], "Tomba! 2"),
            (["tomba2", "--help"], "Tomba! 2"),
        )
        for arguments, expected in cases:
            with self.subTest(arguments=arguments), temporary_directory() as temp:
                completed = subprocess.run(
                    [sys.executable, str(REPO / "bootstrap.py"), *arguments],
                    cwd=temp,
                    env=environment,
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(completed.returncode, 0, completed.stderr)
                self.assertIn(expected, completed.stdout)
                self.assertNotIn("[run] error", completed.stderr)

    def test_shell_shim_enters_repo_and_uses_frozen_uv_environment(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            fake_bin = root / "fake-bin"
            fake_bin.mkdir()
            shim = root / "run.sh"
            shutil.copyfile(REPO / "run.sh", shim)
            shim.chmod(0o755)
            capture = root / "capture.txt"
            fake_uv = fake_bin / "uv"
            fake_uv.write_text(
                '#!/bin/sh\nprintf \'%s\\n\' "$PWD" "$@" > "$LAUNCH_CAPTURE"\n',
                encoding="utf-8",
            )
            fake_uv.chmod(0o755)
            environment = {
                "PATH": f"{fake_bin}{os.pathsep}{os.defpath}",
                "LAUNCH_CAPTURE": str(capture),
            }

            completed = subprocess.run(
                [str(shim), "disc with spaces.chd"], env=environment, check=False
            )

            self.assertEqual(completed.returncode, 0)
            self.assertEqual(
                capture.read_text(encoding="utf-8").splitlines(),
                [
                    str(root),
                    "run",
                    "--frozen",
                    "python",
                    "bootstrap.py",
                    "disc with spaces.chd",
                ],
            )

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

    def test_platform_install_commands_are_actionable(self) -> None:
        self.assertEqual(
            run.install_instruction("sdl3", "fedora"),
            "please run: sudo dnf install SDL3-devel",
        )
        self.assertEqual(
            run.install_instruction("sdl3", "debian"),
            "please run: sudo apt install libsdl3-dev",
        )
        self.assertEqual(
            run.install_instruction("sdl3", "macos"),
            "please run: brew install sdl3",
        )

    def test_host_family_uses_linux_distribution_identity(self) -> None:
        self.assertEqual(run.host_family("Linux", {"ID": "fedora"}), "fedora")
        self.assertEqual(
            run.host_family("Linux", {"ID": "pop", "ID_LIKE": "ubuntu debian"}),
            "debian",
        )

    def test_missing_native_library_names_exact_dnf_install(self) -> None:
        with (
            mock.patch.object(
                run.subprocess,
                "run",
                side_effect=subprocess.CalledProcessError(1, ["pkg-config"]),
            ),
            mock.patch.object(run, "host_family", return_value="fedora"),
            self.assertRaisesRegex(
                run.LauncherError,
                r"SDL3_image \(sdl3-image\).*sudo dnf install SDL3_image-devel",
            ),
        ):
            run.require_pkg_config("sdl3-image", "SDL3_image", Path.cwd())

    def test_zero_argument_flow_builds_project_target_and_execs_it(self) -> None:
        with temporary_directory() as temp:
            root = Path(temp)
            (root / "external/psxport/cmake").mkdir(parents=True)
            (root / "external/psxport/cmake/psxport.cmake").touch()
            framework_build, game_build = run.player_build_dirs(root, "gcc", "g++")
            discdump = framework_build / "tools/discdump"
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

            def player_environment(_: dict[str, str]) -> dict[str, str]:
                return {"PSXPORT_VK_WINDOW": "1"}

            with (
                mock.patch.object(run, "require_native_dependencies"),
                mock.patch.object(run, "run_checked", side_effect=fake_checked),
                mock.patch.object(run, "command_output", return_value=""),
                mock.patch.object(run, "exec_program", side_effect=fake_exec),
                mock.patch.object(run, "processor_count", return_value=8),
                mock.patch.object(
                    run.runpy,
                    "run_path",
                    return_value={"player_environment": player_environment},
                ) as policy_load,
                mock.patch.dict(
                    os.environ,
                    {
                        "CC": "gcc",
                        "CXX": "g++",
                        "PSXPORT_VK_HEADLESS": "1",
                        "PSXPORT_NOAUDIO": "1",
                        "PSXPORT_NOPACE": "1",
                    },
                    clear=True,
                ),
            ):
                self.assertEqual(run.main([], root=root), 1)

            self.assertEqual(
                policy_load.call_args.args,
                (str(root / "external/psxport/tools/port/launch_environment.py"),),
            )

            self.assertIn(
                [
                    "cmake",
                    "--build",
                    str(game_build),
                    "-j",
                    "8",
                    "--target",
                    "tomba2_port",
                ],
                commands,
            )
            self.assertEqual(launched[0], str(root / "build/bin/tomba2_port"))
            self.assertEqual(
                launched[1],
                [str(root / "build/bin/tomba2_port"), "scratch/bin/tomba2/MAIN.EXE"],
            )
            self.assertEqual(launched[2]["PSXPORT_ASSET_DIR"], "external/psxport")
            self.assertEqual(launched[2]["PSXPORT_TOMBA2_DISC"], "./game.chd")
            self.assertEqual(launched[2]["PSXPORT_VK_WINDOW"], "1")
            for agent_key in (
                "PSXPORT_VK_HEADLESS",
                "PSXPORT_NOAUDIO",
                "PSXPORT_NOPACE",
            ):
                self.assertNotIn(agent_key, launched[2])
            flattened = [argument for command in commands for argument in command]
            self.assertIn("-DCMAKE_C_COMPILER=gcc", flattened)
            self.assertIn("-DCMAKE_CXX_COMPILER=g++", flattened)
            self.assertIn(f"-DPython3_EXECUTABLE={run.sys.executable}", flattened)
            self.assertNotIn("ctest", flattened)
            self.assertNotIn("--version", flattened)
            self.assertIn(str(framework_build), flattened)
            self.assertIn(str(game_build), flattened)
            self.assertNotIn(str(root / "build"), flattened)
            self.assertTrue(
                all(
                    "-DBUILD_TESTING=OFF" in command
                    for command in commands
                    if command[:2] == ["cmake", "-S"]
                )
            )

    def test_player_builds_are_toolchain_keyed_and_repo_local(self) -> None:
        clang = run.player_build_dirs(REPO, "clang", "clang++")
        gcc = run.player_build_dirs(REPO, "gcc", "g++")
        self.assertNotEqual(clang, gcc)
        for build in (*clang, *gcc):
            self.assertTrue(build.is_relative_to(REPO / "build/player"))


if __name__ == "__main__":
    unittest.main()
