"""Check Tomba! 1's title-local ownership and source-size boundary."""

from __future__ import annotations

import argparse
import json
import pathlib

TITLE_ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".cc", ".cmake", ".cpp", ".h", ".hh", ".hpp", ".py"}
SOURCE_LINE_CAP = 1200
REQUIRED = (
    "AGENTS.md",
    "enhancement_scope.json",
    "executable.json",
    "cmake/tomba1_port.cmake",
    "docs/codemap.md",
    "docs/project-state.md",
    "docs/re-frontier.md",
    "game/app/README.md",
    "game/app/main.cpp",
    "game/core/README.md",
    "game/core/cd_native_startup.cpp",
    "game/core/cd_native_startup.h",
    "game/core/frame_driver.cpp",
    "game/core/frame_driver.h",
    "game/core/native_boot.cpp",
    "game/core/native_boot.h",
    "game/core/sync_native.cpp",
    "game/core/sync_native.h",
    "game/core/stream_field_turn.cpp",
    "game/core/stream_field_turn.h",
    "game/core/tomba1_runtime.cpp",
    "game/core/tomba1_runtime.h",
    "game/render/README.md",
    "tests/test_stream_field_turn.cpp",
    "tools/compare_crt0_boundary.py",
    "tools/verify_executable.py",
    "tools/provision.py",
)
FORBIDDEN_SOURCE_TOKENS = (
    '"tomba_runtime.h"',
    '"legacy_game_interface.h"',
    '"fps60.h"',
    '"effect_lerp.h"',
    "game_tomba2",
    "TombaRuntime",
    "TombaFrameDriver",
    "tomba::",
    "0x80085900",
)
FORBIDDEN_CAPABILITY_TOKENS = (
    "60fps",
    "fps60",
    "interpolation",
    "lerp",
    "native_renderer",
    "native_rendering",
    "psxport_native_render",
    "psxport_render_native",
    "temporal_frame_history",
)
EXPECTED_SCOPE = {"widescreen": True}


def excluded_capability_tokens(text: str) -> list[str]:
    folded = text.casefold()
    return [token for token in FORBIDDEN_CAPABILITY_TOKENS if token in folded]


def scope_is_widescreen_only(scope: object) -> bool:
    return scope == EXPECTED_SCOPE


def source_is_checked(path: pathlib.Path) -> bool:
    return path.suffix.lower() in SOURCE_SUFFIXES


def run_selftest() -> int:
    checks = (
        (
            "exact widescreen-only scope is accepted",
            scope_is_widescreen_only(EXPECTED_SCOPE.copy()),
        ),
        (
            "an extra unsupported mode produces the opposite scope answer",
            not scope_is_widescreen_only({**EXPECTED_SCOPE, "extra_mode": True}),
        ),
        (
            "native-render option spelling is rejected",
            excluded_capability_tokens("option(PSXPORT_NATIVE_RENDER ON)")
            == ["psxport_native_render"],
        ),
        (
            "lerp source spelling is rejected",
            excluded_capability_tokens("void lerp_camera();") == ["lerp"],
        ),
        (
            "non-source runtime input is excluded from authored-source limits",
            not source_is_checked(TITLE_ROOT / "runtime-images/SCUS_942.36"),
        ),
    )
    failures = [label for label, passed in checks if not passed]
    if failures:
        print(f"FAIL: {len(failures)} title-isolation selftest(s)")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    for label, _passed in checks:
        print(f"PASS: {label}")
    print(f"SELFTEST: {len(checks)}/{len(checks)} checks passed")
    return 0


def check_title() -> int:
    failures: list[str] = []
    for relative in REQUIRED:
        if not (TITLE_ROOT / relative).is_file():
            failures.append(f"missing required title owner: {relative}")

    sources = sorted(
        path for path in TITLE_ROOT.rglob("*") if path.is_file() and source_is_checked(path)
    )
    for path in sources:
        relative = path.relative_to(TITLE_ROOT)
        text = path.read_text(encoding="utf-8", errors="replace")
        lines = len(text.splitlines())
        if lines > SOURCE_LINE_CAP:
            failures.append(
                f"{relative}: {lines} lines exceeds the {SOURCE_LINE_CAP}-line cap"
            )
        if path != pathlib.Path(__file__).resolve():
            for token in FORBIDDEN_SOURCE_TOKENS:
                if token in text:
                    failures.append(
                        f"{relative}: imports Tomba! 2 ownership token {token!r}"
                    )
            for token in excluded_capability_tokens(text):
                failures.append(
                    f"{relative}: exposes excluded Tomba! 1 capability token {token!r}"
                )

    cmake = (TITLE_ROOT / "cmake/tomba1_port.cmake").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in ("${CMAKE_SOURCE_DIR}/game", "tomba2_port", "generated/shard"):
        if token in cmake:
            failures.append(
                f"cmake/tomba1_port.cmake references cross-title path {token!r}"
            )

    application = (TITLE_ROOT / "game/app/main.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    for required in (
        "cfg_str(kDiscEnvironmentKey)",
        "core->runtime->registerOverrides(*game)",
        "shell.prepareProduct(*game)",
        "shell.step(*core, frame)",
    ):
        if required not in application:
            failures.append(f"game/app/main.cpp omits product-loop contract {required!r}")

    try:
        scope = json.loads(
            (TITLE_ROOT / "enhancement_scope.json").read_text(encoding="utf-8")
        )
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"enhancement_scope.json is unreadable: {exc}")
        scope = None
    if scope is not None and not scope_is_widescreen_only(scope):
        failures.append(
            "enhancement_scope.json must contain only the enabled widescreen capability"
        )

    if failures:
        print(f"FAIL: {len(failures)} title-isolation violation(s)")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(
        f"PASS: {len(REQUIRED)}/{len(REQUIRED)} owners present; "
        f"{len(sources)} source file(s) isolated and <= {SOURCE_LINE_CAP} lines; "
        "widescreen-only option contract enforced"
    )
    print(
        "blind spot: this structural check does not prove executable behavior or widescreen correctness"
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return run_selftest()
    return check_title()


if __name__ == "__main__":
    raise SystemExit(main())
