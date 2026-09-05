#!/usr/bin/env python3
"""Refuse retired guest-source execution artifacts across the maintained tree."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import Counter
from collections.abc import Callable, Iterable, Sequence
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_PATHS = (
    "game/core/field_owned_leaves.cpp",
    "game/core/announcer_cue_push.cpp",
    "game/core/engine_field_transition.cpp",
    "game/core/field_seq_scheduler.cpp",
    "game/core/field_target_cursor.cpp",
    "game/core/spawn_type6_node.cpp",
    "game/ai/contact_stamp.cpp",
    "game/ai/cull_substate_native.cpp",
    "game/input/input.cpp",
    "game/input/pad_sampler.cpp",
    "game/player/actor_tomba_actions.cpp",
    "game/player/actor_tomba_pretick.cpp",
    "game/player/actor_tomba_action_800531dc.cpp",
    "game/player/actor_tomba_action_800588bc.cpp",
    "game/player/actor_tomba_action_8005accc.cpp",
    "game/player/actor_tomba_action_8005aee4.cpp",
    "game/player/actor_tomba_action_8005ef48.cpp",
    "game/player/actor_tomba_action_8005f1b0.cpp",
    "game/player/actor_tomba_action_800660ac.cpp",
    "game/render/ui_ft4_layout.cpp",
    "game/ui/dialog_advance.cpp",
    "game/ui/dialog_box_sm.cpp",
    "game/ui/dialog_driver.cpp",
    "game/ui/dialog_driver_sibling.cpp",
    "generated",
    "game/core/recomp_register.cpp",
    "game/core/recomp_register.h",
    "game/recomp_seeds.json",
    "tools/ensure_recomp.py",
    "titles/tomba1/generated",
    "titles/tomba1/game/core/recomp_register.cpp",
    "titles/tomba1/game/core/recomp_register.h",
    "titles/tomba1/game/recomp_seeds.json",
    "titles/tomba1/tools/ensure_recomp.py",
    "tools/codemap_overrides.tsv",
    "docs/re/render-targets-static-re.md",
    "docs/kanban/cards/006-load-game-browser-demo-s48-4-aborts-rec-dispatch.md",
    "docs/kanban/cards/024-area-22-aborts-on-entry-rec-dispatch-miss-0x8010.md",
    "docs/kanban/cards/027-recomp-misread-jump-table-base-blocked-four-area.md",
)
FORBIDDEN_TERMS = re.compile(
    r"\b(?:rec_dispatch|rec_super_call|rec_interp|recomp_register|tomba_install_recomp|MV_CHECK|"
    r"main_dispatch|shard_set_override|engine_set_override_[A-Za-z0-9_]*|"
    r"[A-Za-z0-9_]+_set_override|gen_func_[0-9A-Fa-f]+|"
    r"ov_[A-Za-z0-9_]+_(?:gen|func)_[0-9A-Fa-f]+|func_[0-9A-Fa-f]{8})\b|"
    r"\b(?:rc[0-4]|guest_fn|guest_leaf|guest_dispatch|guest_call)\s*(?:<|\()|"
    r"\boverrides::install\b|runtime/(?:recomp|retired source path)/|"
    r"\b(?:g_override|g_ov_[A-Za-z0-9_]+_override)\b|"
    r"\b(?:port_gen|gen_annotate|advanceByteGen)\b|"
    r'[#]include\s+["<](?:override_registry|recomp_iface|recomp_register|rec_decls|'
    r'[A-Za-z0-9_]+_decls)\.h[">]'
)
FORBIDDEN_RUNTIME_TERMS = re.compile(r"\b(?:psx_fallback|diff_mode|native_gates)\b")
FORBIDDEN_GENERATED_OWNERS = re.compile(
    r"^\s*(?:(?:static\s+)?void\s+)?(?:register_field_owned_leaves\s*\(|"
    r"(?:ContactStamp|CullSubstateLeaves|PadSampler|UiFt4Layout)::registerOverrides\s*\(|"
    r"ActorTomba::(?:gov_)?(?:actionHandler800[0-9A-Fa-f]+|mode0WalkHandler|"
    r"matrixComposeAttached|enterOuterState0|proximityAngleWalk|limbFrameLoad|"
    r"invincibilityFlashStep|rampOffsetStep)\s*\()"
)
RUNTIME_SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp"}

ALLOWED_POLICY_FILES = {
    Path("docs/migration.md"),
    Path("tools/verify_dynarec_boundary.py"),
    Path("titles/tomba1/tools/verify_title_isolation.py"),
}
TEXT_SUFFIXES = {
    "",
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".h",
    ".hpp",
    ".json",
    ".md",
    ".py",
    ".sh",
    ".txt",
    ".yaml",
    ".yml",
}


@dataclass(frozen=True)
class Violation:
    path: Path
    line: int
    token: str


def maintained_files(root: Path) -> Iterable[Path]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    for relative_text in sorted(set(result.stdout.splitlines())):
        relative = Path(relative_text)
        if not relative_text or relative in ALLOWED_POLICY_FILES or relative.parts[:1] == ("external",):
            continue
        path = root / relative
        if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES:
            yield path


def forbidden_paths_present(root: Path, exists: Callable[[Path], bool] = Path.exists) -> tuple[Path, ...]:
    return tuple(Path(item) for item in FORBIDDEN_PATHS if exists(root / item))


def scan(root: Path) -> tuple[int, tuple[Violation, ...], tuple[Path, ...]]:
    sources = tuple(maintained_files(root))
    violations: list[Violation] = []
    for path in sources:
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for line, source in enumerate(text.splitlines(), 1):
            for match in FORBIDDEN_TERMS.finditer(source):
                violations.append(Violation(path.relative_to(root), line, match.group(0)))
            if path.suffix.lower() in RUNTIME_SOURCE_SUFFIXES:
                for match in FORBIDDEN_RUNTIME_TERMS.finditer(source):
                    violations.append(Violation(path.relative_to(root), line, match.group(0)))
                for match in FORBIDDEN_GENERATED_OWNERS.finditer(source):
                    violations.append(Violation(path.relative_to(root), line, match.group(0)))
    present_paths = forbidden_paths_present(root)
    adapter = root / "game/core/guest_jal.h"
    if adapter.is_file():
        adapter_text = adapter.read_text(encoding="utf-8")
        required = (
            '#include "guest_call.h"',
            "psx::cpu::dispatchGuestWithArgumentsToReturn(",
            "psx::cpu::ExecutionBudget::currentTurn(core)",
        )
        for token in required:
            if token not in adapter_text:
                violations.append(Violation(adapter.relative_to(root), 1, f"missing {token}"))
        for pattern in (
            re.compile(r"\brec_dispatch\b"),
            re.compile(r"\b(?:gen_)?func_[0-9A-Fa-f]{8}\b"),
            re.compile(r"\binterpreter\b"),
        ):
            match = pattern.search(adapter_text)
            if match:
                line = adapter_text[: match.start()].count("\n") + 1
                violations.append(Violation(adapter.relative_to(root), line, match.group(0)))
    else:
        violations.append(Violation(Path("game/core/guest_jal.h"), 0, "missing typed PSXPort adapter"))
    return len(sources), tuple(violations), present_paths


def report(root: Path) -> int:
    scanned, violations, present_paths = scan(root)
    if not violations and not present_paths:
        print(f"PASS: scanned {scanned} maintained text files; matched 0 retired terms or paths")
        return 0

    print(
        f"REFUSED: scanned {scanned} maintained text files; matched {len(violations)} retired terms "
        f"and {len(present_paths)} retired paths",
        file=sys.stderr,
    )
    for path in present_paths:
        print(f"path: {path}", file=sys.stderr)
    counts = Counter(violation.path for violation in violations)
    for path, count in sorted(counts.items(), key=lambda item: str(item[0])):
        first = next(violation for violation in violations if violation.path == path)
        print(
            f"source: {path}:{first.line}: {count} match(es), first token {first.token}",
            file=sys.stderr,
        )
    return 1


def selftest() -> int:
    output_path = Path("game/core/field_owned_leaves.cpp")
    found = forbidden_paths_present(Path("fixture"), lambda path: path == Path("fixture") / output_path)
    owner = "void register_field_owned_leaves() { }"
    if found != (output_path,) or not FORBIDDEN_GENERATED_OWNERS.search(owner):
        print("SELFTEST FAIL: audited generated output or registration was not refused", file=sys.stderr)
        return 1
    if FORBIDDEN_GENERATED_OWNERS.search("// Historical caller ActorTomba::matrixComposeAttached (guest image)"):
        print("SELFTEST FAIL: historical evidence was mistaken for an executable owner", file=sys.stderr)
        return 1
    if forbidden_paths_present(Path("fixture"), lambda _: False):
        print("SELFTEST FAIL: absent generated output was reported present", file=sys.stderr)
        return 1
    for allowed in ("image-qualified native registration", "original-binary evidence", "ordinary guest bodies"):
        if FORBIDDEN_TERMS.search(allowed):
            print(f"SELFTEST FAIL: harmless runtime/evidence wording refused: {allowed}", file=sys.stderr)
            return 1
    sample = """A stale workflow says rec_dispatch and func_80010000 are historical evidence.
#include "guest_call.h"
void f() {
  guest_call(c, 0x80010000u);
  func_80010000(c);
}
"""
    live = [
        (line, match.group(0))
        for line, source in enumerate(sample.splitlines(), 1)
        for match in FORBIDDEN_TERMS.finditer(source)
    ]
    expected = [(1, "rec_dispatch"), (1, "func_80010000"), (4, "guest_call("), (5, "func_80010000")]
    if live != expected:
        print(f"SELFTEST FAIL: expected documentation and code violations {expected}, got {live}", file=sys.stderr)
        return 1
    runtime_sample = "bool stale = game.psx_fallback || game.diff_mode || game.native_gates.get();"
    runtime_live = [match.group(0) for match in FORBIDDEN_RUNTIME_TERMS.finditer(runtime_sample)]
    runtime_expected = ["psx_fallback", "diff_mode", "native_gates"]
    if runtime_live != runtime_expected:
        print(f"SELFTEST FAIL: expected retired runtime terms {runtime_expected}, got {runtime_live}", file=sys.stderr)
        return 1
    print("SELFTEST PASS: stale documentation and live retired calls are both detected")
    return 0


def parse_args(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_args(arguments)
    return selftest() if args.selftest else report(args.root.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
