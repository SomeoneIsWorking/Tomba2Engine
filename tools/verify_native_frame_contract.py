#!/usr/bin/env python3
"""Verify Tomba's native-owned frame and fatal guest-VSync product contract."""

from __future__ import annotations

import argparse
import pathlib
import re

REPO = pathlib.Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".cc", ".cmake", ".cpp", ".h", ".hh", ".hpp", ".py"}
GUEST_VSYNC_CALLS = (
    re.compile(r"\b(?:Timing::)?vsync(?:Hle|Callback)?\s*\("),
    re.compile(
        r"\bpsx::cpu::dispatchGuest(?:ToReturn)?[0-4]\s*\([^;\n]*0x80085900",
        re.IGNORECASE,
    ),
)
FRAME_ORDER = (
    "core.rsub.otAttr.beginLogicFrame(frame)",
    "autoDrive_.beforeFrame(core, frame)",
    "bindFrameHardware(core)",
    "game.timing.frameTick()",
    "preparePrimarySingleBuffer(core, cfg, envp)",
    "game.pad.serviceFrame()",
    'game.cd.audioTrace("pre")',
    "eng(&core).frameUpdate()",
    "game.presentation.commit(&core, 0, game.temporalPresentation.get())",
    "rend(&core)->bbFrameReset()",
    "applyArmedStandaloneWarp(core, frame)",
    'game.cd.audioTrace("post")',
    "game.pcSched.step()",
    "eng(&core).musicCoord.tick()",
    "psx::cpu::dispatchGuestToReturn1(core, cfg.drawSync, 0",
    "submitFrame(core, cfg, envp)",
    "game.perf.frameEnd()",
    "diagnostics_.afterFrame(core, frame)",
    "autoDrive_.afterFrame(core, frame)",
)
PRIMARY_BUFFER_ORDER = (
    "core.mem_w32(cfg.otBasePtr, envp)",
    "psx::cpu::dispatchGuestToReturn2(",
    "resetSingleBufferTail(core, cfg)",
)
BUFFER_TAIL_ORDER = (
    "core.mem_w16(cfg.dwellCounter, 0)",
    "core.mem_w32(cfg.poolPtrLast, core.mem_r32(cfg.poolPtrCur))",
    "core.mem_w32(cfg.poolPtrCur, cfg.packetPoolBase & 0xffffffu)",
)


def guest_vsync_calls(text: str) -> list[str]:
    return [pattern.pattern for pattern in GUEST_VSYNC_CALLS if pattern.search(text)]


def ordered_tokens(text: str, tokens: tuple[str, ...]) -> bool:
    offset = 0
    for token in tokens:
        found = text.find(token, offset)
        if found < 0:
            return False
        offset = found + len(token)
    return True


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        return ""
    opening = text.find("{", start + len(signature))
    if opening < 0:
        return ""
    depth = 0
    for offset in range(opening, len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return text[opening + 1 : offset]
    return ""


def finite_boot_body(text: str) -> bool:
    return not re.search(r"\b(?:for|while)\s*\(", text) and "FrameLoopShell" not in text and "stepFrame(" not in text


def run_selftest() -> int:
    checks = (
        ("ordinary native timing is accepted", not guest_vsync_calls("game.timing.frameTick();")),
        ("framework Timing::vsync call is rejected", bool(guest_vsync_calls("Timing::vsyncHle(c);"))),
        (
            "typed address-dispatched VSync call is rejected",
            bool(
                guest_vsync_calls(
                    "psx::cpu::dispatchGuestToReturn1(*c, 0x80085900u, 0, budget, owner);"
                )
            ),
        ),
        ("measured frame order is accepted", ordered_tokens(" ".join(FRAME_ORDER), FRAME_ORDER)),
        ("reversed frame order produces the opposite answer", not ordered_tokens(" ".join(reversed(FRAME_ORDER)), FRAME_ORDER)),
        ("primary OT publish-before-clear order is accepted", ordered_tokens(" ".join(PRIMARY_BUFFER_ORDER), PRIMARY_BUFFER_ORDER)),
        (
            "primary clear-before-publish produces the opposite answer",
            not ordered_tokens(
                "psx::cpu::dispatchGuestToReturn2( core, cfg.clearOtagR, envp, 0x800 "
                "core.mem_w32(cfg.otBasePtr, envp) resetSingleBufferTail(core, cfg)",
                PRIMARY_BUFFER_ORDER,
            ),
        ),
        (
            "finite boot prefix is accepted",
            finite_boot_body("psx::cpu::dispatchGuestToReturn0(*c, init, budget, owner); return;"),
        ),
        ("boot-owned frame loop produces the opposite answer", not finite_boot_body("while (running) { stepFrame(c); }")),
    )
    failures = [label for label, passed in checks if not passed]
    if failures:
        print(f"FAIL: {len(failures)} native-frame selftest(s)")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    for label, _passed in checks:
        print(f"PASS: {label}")
    print(f"SELFTEST: {len(checks)}/{len(checks)} checks passed")
    return 0


def check_contract() -> int:
    failures: list[str] = []
    runtime_h = (REPO / "game/core/tomba_runtime.h").read_text(encoding="utf-8")
    runtime_cpp = (REPO / "game/core/tomba_runtime.cpp").read_text(encoding="utf-8")
    driver = (REPO / "game/core/frame_driver.cpp").read_text(encoding="utf-8")
    auto_drive = (REPO / "game/core/auto_drive.cpp").read_text(encoding="utf-8")
    engine = (REPO / "game/game_tomba2.cpp").read_text(encoding="utf-8")
    config = (REPO / "game/core/game_config.cpp").read_text(encoding="utf-8")
    gate = (REPO / "tools/gate.py").read_text(encoding="utf-8")
    framework_boot = (REPO / "external/psxport/runtime/psx/native_boot.cpp").read_text(encoding="utf-8")

    if "createFrameDriver(Game &game) override" not in runtime_h:
        failures.append("TombaRuntime does not declare its FrameDriver factory")
    if "std::make_unique<TombaFrameDriver>(game)" not in runtime_cpp:
        failures.append("TombaRuntime does not create the title-local TombaFrameDriver")
    if "c->game->timing.vsyncCallback()" in runtime_cpp:
        failures.append("bootInit still calls the obsolete successful VSyncCallback no-op")
    if "dualview" in runtime_cpp.lower() or "dualview" in driver.lower():
        failures.append("retired dual-view selection remains in the title runtime or frame driver")
    boot_marker = "void TombaRuntime::bootInit(Core &core)"
    boot = runtime_cpp.split(boot_marker, maxsplit=1)[1] if boot_marker in runtime_cpp else ""
    if not boot or not finite_boot_body(boot):
        failures.append("TombaRuntime::bootInit is not a finite initialization prefix")
    if config.count(".vsyncTrap = 0x80085900u") != 1:
        failures.append("Tomba! 2 must declare exactly one measured fatal VSync trap at 0x80085900")

    step_marker = "void TombaFrameDriver::stepFrame(Core &core, uint32_t frame)"
    if step_marker not in driver:
        failures.append("TombaFrameDriver has no finite stepFrame implementation")
        step = ""
    else:
        step = driver.split(step_marker, maxsplit=1)[1]
    if step.count("game.presentation.commit(") != 1:
        failures.append("TombaFrameDriver must own exactly one presentation commit")
    if not ordered_tokens(step, FRAME_ORDER):
        failures.append("TombaFrameDriver does not preserve the measured native frame order")
    primary = function_body(driver, "void preparePrimarySingleBuffer(Core &core, const GameConfig &cfg, uint32_t envp)")
    tail = function_body(driver, "void resetSingleBufferTail(Core &core, const GameConfig &cfg)")
    if not primary or "cfg.clearOtagR" not in primary or not ordered_tokens(primary, PRIMARY_BUFFER_ORDER):
        failures.append("primary buffer preparation must publish otBasePtr before ClearOTagR")
    if not tail or not ordered_tokens(tail, BUFFER_TAIL_ORDER):
        failures.append("shared single-buffer tail does not preserve dwell/pool order")
    if ".presentation.commit(" in engine or "frame_commit(" in engine:
        failures.append("Engine::frameUpdate still owns a second presentation boundary")
    if not ordered_tokens(
        auto_drive,
        (
            "game.repl.navNewgame = 0",
            "game.repl.requestPrompt()",
        ),
    ):
        failures.append("Tomba auto-drive does not request the next generic REPL prompt at the GAME boundary")

    for token in (
        "otAttr.beginLogicFrame",
        "navNewgame",
        "skipFrames",
        "0x1F800137",
        "0x80109450",
    ):
        if token in framework_boot:
            failures.append(f"shared native_boot.cpp still owns Tomba! 2 frame policy {token!r}")

    for path in sorted((REPO / "game").rglob("*")):
        if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern in guest_vsync_calls(text):
            failures.append(f"{path.relative_to(REPO)} successfully calls guest VSync via /{pattern}/")

    for required in (r"VSync:\s*timeout", r"GUEST VSYNC VIOLATION"):
        if required not in gate:
            failures.append(f"tools/gate.py does not reject product output matching {required!r}")

    tomba1_sources = [
        path
        for path in (REPO / "titles/tomba1").rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    ]
    for path in sorted(tomba1_sources):
        text = path.read_text(encoding="utf-8", errors="replace")
        for token in ("0x80085900", "TombaFrameDriver"):
            if token in text and path.name != "verify_title_isolation.py":
                failures.append(f"{path.relative_to(REPO)} reuses Tomba! 2 frame fact {token!r}")

    if failures:
        print(f"FAIL: {len(failures)} native-frame contract violation(s)")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(
        "PASS: Tomba! 2 owns one finite ordered frame transaction and one presentation fence; "
        "0x80085900 remains fatal; Tomba! 1 reuses no Tomba! 2 frame facts"
    )
    print(f"denominator: scanned {len(tomba1_sources)} Tomba! 1 source file(s) plus every root game source")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    return run_selftest() if args.selftest else check_contract()


if __name__ == "__main__":
    raise SystemExit(main())
