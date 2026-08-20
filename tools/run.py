#!/usr/bin/env python3
"""Build and launch the Tomba! 2 native PC port."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from collections.abc import Sequence
from pathlib import Path
from typing import NoReturn

ROOT = Path(__file__).resolve().parents[1]
CYAN = "\033[1;36m"
RED = "\033[1;31m"
RESET = "\033[0m"


class LauncherError(RuntimeError):
    """An actionable launcher refusal."""


def say(message: str) -> None:
    print(f"{CYAN}[run]{RESET} {message}", flush=True)


def run_checked(
    command: Sequence[str],
    *,
    root: Path,
    error: str,
    env: dict[str, str] | None = None,
    quiet: bool = False,
) -> None:
    try:
        subprocess.run(
            list(command),
            cwd=root,
            env=env,
            stdout=subprocess.DEVNULL if quiet else None,
            check=True,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise LauncherError(error) from exc


def command_output(command: Sequence[str], *, root: Path) -> str:
    try:
        return subprocess.run(
            list(command),
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def exec_program(
    program: str, argv: Sequence[str], env: dict[str, str], root: Path
) -> NoReturn:
    try:
        os.chdir(root)
        os.execvpe(program, list(argv), env)
    except OSError as exc:
        raise LauncherError(f"could not launch {program}: {exc}") from exc


def require_tool(name: str, hint: str) -> None:
    if shutil.which(name) is None:
        raise LauncherError(f"{name} not found{hint}")


def validate_toolchain(cc: str, cxx: str, root: Path) -> None:
    for variable, compiler in (("CC", cc), ("CXX", cxx)):
        version = command_output([compiler, "--version"], root=root).lower()
        if "clang" not in version:
            raise LauncherError(f"{variable}={compiler} is not Clang")


def processor_count() -> int:
    return os.cpu_count() or 4


def parse_arguments(arguments: Sequence[str]) -> tuple[str | None, list[str]]:
    remaining = list(arguments)
    resume: str | None = None
    if remaining and remaining[0] == "--resume":
        remaining.pop(0)
        if (
            remaining
            and not remaining[0].startswith("-")
            and remaining[0].endswith(".pad")
        ):
            resume = remaining.pop(0)
        else:
            resume = ""
    return resume, remaining


def resolve_resume(requested: str | None, root: Path, env: dict[str, str]) -> None:
    if requested is None:
        return
    if requested:
        display = requested
        resume = Path(display)
    else:
        last = root / "scratch/bin/pad_session.pad"
        if not last.is_file():
            raise LauncherError(
                "--resume: no recording at scratch/bin/pad_session.pad yet — "
                "play a windowed session first, or pass a .pad"
            )
        display = "scratch/bin/pad_resume.pad"
        target = root / display
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(last, target)
        resume = Path(display)

    check_path = resume if resume.is_absolute() else root / resume
    if not check_path.is_file():
        raise LauncherError(f"--resume: no such recording: {resume}")
    frames = check_path.stat().st_size // 2
    say(f"resume: {display} ({frames} pad frames) — fast-forwarding, then it is yours")
    env["PSXPORT_PAD_RESUME"] = display


def env_file_value(path: Path, key: str) -> str:
    if not path.is_file():
        return ""
    pattern = re.compile(rf"^\s*{re.escape(key)}\s*=\s*(.*)$")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            return match.group(1)
    return ""


def resolve_disc(argument: str | None, root: Path, env: dict[str, str]) -> str:
    disc = argument or env.get("PSXPORT_TOMBA2_DISC", "")
    if not disc:
        disc = env_file_value(root / ".env", "PSXPORT_TOMBA2_DISC")
    if not disc:
        disc = env_file_value(root / ".env", "PSXPORT_DISC")
    if not disc:
        drops = sorted(root.glob("*.chd"))
        disc = f"./{drops[0].name}" if drops else ""
    path = Path(disc)
    if not disc or not (path if path.is_absolute() else root / path).is_file():
        raise LauncherError(
            "no disc image — pass it as ./run.sh <disc.chd>, set PSXPORT_TOMBA2_DISC, "
            "or drop a *.chd here"
        )
    return disc


def framework_status(psxport: Path, root: Path, explicit: bool) -> str:
    commit = (
        command_output(
            ["git", "-C", str(psxport), "rev-parse", "--short", "HEAD"], root=root
        )
        or "?"
    )
    dirty = (
        " +dirty"
        if command_output(
            ["git", "-C", str(psxport), "status", "--porcelain"], root=root
        )
        else ""
    )
    if explicit:
        return f"framework: *** {psxport} *** (DEV CLONE {commit}{dirty}) — NOT the recorded pin"
    resolved = (
        psxport.resolve(strict=False)
        if psxport.is_absolute()
        else (root / psxport).resolve(strict=False)
    )
    return f"framework: external/psxport -> {resolved} @ {commit}{dirty}"


def sync_framework(root: Path, env: dict[str, str]) -> Path:
    run_checked(
        [sys.executable, "tools/psxport_sync.py", "--auto"],
        root=root,
        error="could not resolve external/psxport",
        env=env,
    )
    explicit = bool(env.get("PSXPORT_DIR"))
    psxport = Path(env.get("PSXPORT_DIR", "external/psxport"))
    check_root = psxport if psxport.is_absolute() else root / psxport
    if not (check_root / "cmake/psxport.cmake").is_file():
        raise LauncherError(f"PSXPORT_DIR={psxport} is not a psxport checkout")
    say(framework_status(psxport, root, explicit))

    if (root / ".gitmodules").is_file() and shutil.which("git"):
        sync_script = root / "external/psxport/scripts/sync-submodules.sh"
        if not sync_script.is_file():
            say("initializing git submodules…")
            run_checked(
                ["git", "submodule", "update", "--init", "--recursive"],
                root=root,
                error="git submodule update failed",
            )
        if sync_script.is_file():
            run_checked(
                ["bash", str(sync_script)], root=root, error="submodule sync failed"
            )
        else:
            say(
                "WARNING: external/psxport/scripts/sync-submodules.sh is absent even after init —"
            )
            say(
                "         submodules were NOT synced and may not match this repo's recorded gitlinks."
            )
    return psxport


def configure_and_build(
    root: Path,
    psxport: Path,
    cc: str,
    cxx: str,
    jobs: int,
    env: dict[str, str],
) -> Path:
    psxport_build = psxport / "build"
    say("building libchdr + discdump (CMake)…")
    run_checked(
        [
            "cmake",
            "-S",
            str(psxport),
            "-B",
            str(psxport_build),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_C_COMPILER={cc}",
            f"-DCMAKE_CXX_COMPILER={cxx}",
        ],
        root=root,
        error="psxport cmake configure failed",
        env=env,
        quiet=True,
    )
    run_checked(
        [
            "cmake",
            "--build",
            str(psxport_build),
            "-j",
            str(jobs),
            "--target",
            "discdump",
        ],
        root=root,
        error="discdump build failed",
        env=env,
        quiet=True,
    )
    discdump = psxport_build / "tools/discdump"
    check_discdump = discdump if discdump.is_absolute() else root / discdump
    if not check_discdump.is_file():
        discdump = Path(f"{discdump}.exe")
        check_discdump = discdump if discdump.is_absolute() else root / discdump
    if not os.access(check_discdump, os.X_OK):
        raise LauncherError("discdump build failed")
    return discdump


def provision_and_build_game(
    disc: str,
    discdump: Path,
    psxport: Path,
    cc: str,
    cxx: str,
    jobs: int,
    root: Path,
    env: dict[str, str],
) -> Path:
    main_exe = Path("scratch/bin/tomba2/MAIN.EXE")
    (root / "generated").mkdir(parents=True, exist_ok=True)
    (root / "scratch/bin").mkdir(parents=True, exist_ok=True)
    provision_env = env | {
        "PSXPORT_DISCDUMP": str(discdump),
        "PSXPORT_DIR": str(psxport),
    }
    run_checked(
        [sys.executable, "tools/ensure_recomp.py", disc],
        root=root,
        error="recomp provisioning failed",
        env=provision_env,
    )
    if not (root / main_exe).is_file():
        raise LauncherError("ensure_recomp.py did not produce MAIN.EXE")

    say(f"building the native port (CMake -j{jobs})…")
    psxport_absolute = (
        psxport.resolve() if psxport.is_absolute() else (root / psxport).resolve()
    )
    run_checked(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            "build",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DPSXPORT_DIR={psxport_absolute}",
            f"-DCMAKE_C_COMPILER={cc}",
            f"-DCMAKE_CXX_COMPILER={cxx}",
        ],
        root=root,
        error="cmake configure failed",
        env=env,
        quiet=True,
    )
    run_checked(
        ["cmake", "--build", "build", "-j", str(jobs), "--target", "tomba2_port"],
        root=root,
        error="port build failed",
        env=env,
    )
    return main_exe


def main(arguments: Sequence[str] | None = None, *, root: Path = ROOT) -> int:
    args = list(sys.argv[1:] if arguments is None else arguments)
    env = dict(os.environ)
    try:
        require_tool("cmake", " (macOS: brew install cmake)")
        require_tool("pkg-config", " (macOS: brew install pkg-config)")
        run_checked(
            ["pkg-config", "--exists", "sdl3"],
            root=root,
            error="SDL3 not found (macOS: brew install sdl3; Linux: SDL3-devel / libsdl3-dev)",
        )
        cc = env.get("CC", "clang")
        cxx = env.get("CXX", "clang++")
        validate_toolchain(cc, cxx, root)

        psxport = sync_framework(root, env)
        requested_resume, remaining = parse_arguments(args)
        resolve_resume(requested_resume, root, env)
        disc = resolve_disc(remaining[0] if remaining else None, root, env)
        say(f"disc: {disc}")

        jobs = processor_count()
        discdump = configure_and_build(root, psxport, cc, cxx, jobs, env)
        main_exe = provision_and_build_game(
            disc, discdump, psxport, cc, cxx, jobs, root, env
        )

        say("launching Tomba! 2 (native PC port)…")
        if env.get("PSXPORT_NOWINDOW"):
            env["PSXPORT_VK_HEADLESS"] = "1"
        else:
            env["PSXPORT_VK_WINDOW"] = "1"
        env["PSXPORT_ASSET_DIR"] = env.get("PSXPORT_ASSET_DIR") or str(psxport)
        env["PSXPORT_DEBUG_SERVER"] = env.get("PSXPORT_DEBUG_SERVER") or "1"
        env["PSXPORT_NO_TERRAIN"] = env.get("PSXPORT_NO_TERRAIN") or "0"
        env["PSXPORT_TOMBA2_DISC"] = disc
        exec_program(
            "./scratch/bin/tomba2_port",
            ["./scratch/bin/tomba2_port", str(main_exe)],
            env,
            root,
        )
    except LauncherError as exc:
        print(f"{RED}[run] error:{RESET} {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
