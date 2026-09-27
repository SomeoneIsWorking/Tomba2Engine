"""Show that the Tomba! 1 widescreen tests can FAIL: mutate the owner, one defect at a time.

Each mutant is a plausible wrong implementation of the same claim, not a random edit:
  M1 leaves OFX at the retail centre. The picture then renders at 4:3 inside a 428-wide canvas,
     which is a zoom rather than the FOV widening docs/presentation-contract.md describes.
  M2 forgets to widen the guest draw clip.
  M3 drops the "no draw environment before a projection" refusal.
  M4 re-asserts the centre on every frame instead of only when it changed.
  M5 folds ASPECT_AUTO to 16:9 (a run that reports wide while resolving to 4:3).
  M6 hardcodes the 320x224 draw width and ignores the width the guest published, so the
     640x480 mode widens to 428.
  M7 moves OFY along with OFX.
  M8 accepts any plan the latch returns, including an unusable one.
  M9 skips the retail SetDefDrawEnv body, leaving the environment the guest reads unwritten.
  M10 remembers the WIDENED width rather than the one measured, so the next frame re-latches from a
     width the guest never published and the picture widens a second time.
  M11 computes the plan correctly and then does not keep it, so every reader of the stored plan
     sees a default-constructed zero plan.

Run: uv run --frozen python titles/tomba1/tools/mutation_check.py
"""

from __future__ import annotations

import pathlib
import subprocess
import sys

REPOSITORY = pathlib.Path(__file__).resolve().parents[3]
SOURCE = REPOSITORY / "titles/tomba1/game/render/widescreen_projection.cpp"
TEST = "./build/tomba1_widescreen_projection_test"
BUILD = ["cmake", "--build", "build", "--target", "tomba1_widescreen_projection_test"]

MUTANTS: dict[str, tuple[str, str]] = {
    "M1 OFX left at retail (a zoom)": (
        "const auto centerX = latched.projectionCenterX;",
        "const auto centerX = static_cast<int>(latched.nativeProjectionExtent.width) / 2;",
    ),
    "M2 draw clip never widened": (
        "core.r[kArgumentA3] = static_cast<std::uint32_t>(latched.guestDrawWidth);",
        "core.r[kArgumentA3] = static_cast<std::uint32_t>(retailDrawWidth);",
    ),
    "M3 unpaired draw environment allowed": (
        """  if (!projectionPublished_) {
    // X4 refuses the same unpaired publication, and for the same reason: widening a draw clip
    // against a projection the title never stated is a clip with nothing to centre it in.
    refuse("draw-environment publication ran before 0x80063A34 stated a projection");
  }
""",
        "",
    ),
    "M4 centre re-asserted every frame": (
        """  if (latched.projectionCenterX == appliedCenterX_) {
    return;
  }
""",
        "",
    ),
    "M5 ASPECT_AUTO folded to 16:9": (
        "  case ASPECT_AUTO:\n    return PresentationAspect::MatchSink;",
        "  case ASPECT_AUTO:\n    return PresentationAspect::Wide16x9;",
    ),
    "M6 draw width hardcoded, guest ignored": (
        "return {kMeasured320x224.extent, static_cast<int>(drawWidth)};",
        "return kMeasured320x224;",
    ),
    "M7 OFY moved with OFX": (
        "libgte_set_geom_offset(&core, centerX, verticalOffset_);",
        "libgte_set_geom_offset(&core, centerX, centerX);",
    ),
    "M8 plan validation removed": (
        "  if (latched.projectionCenterX <= 0 || latched.guestDrawWidth <= 0 || latched.guestDrawWidth > kWidestGuestClip) {",
        "  if (false) {",
    ),
    "M9 retail leaf body never run": (
        "  retail(core);",
        "  // the retail body was skipped, leaving the draw environment unwritten",
    ),
    "M10 remembered draw width is the widened one": (
        "  drawWidth_ = retailDrawWidth;",
        "  drawWidth_ = latched.guestDrawWidth;",
    ),
    # The STORE, not just the arithmetic. A validator that computed the right plan and then failed to
    # keep it would leave every reader of `plan()` reading a default-constructed zero plan, which is
    # a different defect from computing the wrong one and would survive a test that only checks the
    # register side effects.
    "M11 latched plan never stored": ("  plan_ = latched;", "  plan_ = GuestProjectionPlan{};"),
}


def run(argv: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        argv, cwd=REPOSITORY, capture_output=True, text=True, timeout=2400, check=False
    )


def first_failure(output: str) -> str:
    for line in output.splitlines():
        if "FAIL" in line or "NEGATIVE FAILED" in line:
            return line.strip()[:150]
    return ""


def main() -> int:
    if not (REPOSITORY / TEST).is_file():
        print(f"REFUSED: {TEST} is absent; build the focused test first")
        return 2
    original = SOURCE.read_text(encoding="utf-8")

    print("BASELINE: rebuilding the unmutated owner — a stale binary would report the PREVIOUS mutant's")
    print("          verdict as this run's, which is how a killed mutant becomes a phantom pass")
    if run(BUILD).returncode != 0:
        print("REFUSED: the unmutated owner does not build")
        return 2
    baseline = run([TEST])
    if baseline.returncode != 0:
        print(f"REFUSED: the unmutated owner already fails: {first_failure(baseline.stdout + baseline.stderr)}")
        return 2
    print("BASELINE: PASS")

    rows: list[tuple[str, str, str]] = []
    try:
        for name, (old, new) in MUTANTS.items():
            if old not in original:
                rows.append((name, "PATTERN NOT FOUND", ""))
                continue
            SOURCE.write_text(original.replace(old, new, 1), encoding="utf-8")
            build = run(BUILD)
            if build.returncode != 0:
                rows.append((name, "did not compile", build.stderr.strip().splitlines()[-1][:150]))
                continue
            result = run([TEST])
            if result.returncode == 0:
                rows.append((name, "*** SURVIVED ***", "the test cannot see this defect"))
            else:
                rows.append((name, f"killed (exit {result.returncode})", first_failure(result.stdout + result.stderr)))
    finally:
        SOURCE.write_text(original, encoding="utf-8")

    width = max(len(name) for name in MUTANTS)
    for name, verdict, detail in rows:
        print(f"{verdict:24s} {name:{width}s}  {detail}")
    survivors = [name for name, verdict, _ in rows if verdict.startswith("***")]
    unmeasured = [name for name, verdict, _ in rows if verdict in ("PATTERN NOT FOUND", "did not compile")]
    print()
    print(f"MUTANTS: {len(MUTANTS)} attempted, {len(MUTANTS) - len(survivors) - len(unmeasured)} killed, "
          f"{len(survivors)} survived, {len(unmeasured)} not measurable")
    if survivors or unmeasured:
        print("A test that cannot fail is worse than none: this run is incomplete.")
        return 1

    restore = run(BUILD)
    if restore.returncode != 0:
        print("REFUSED: the restored source does not rebuild")
        return 2
    final = run([TEST])
    if final.returncode != 0:
        print("REFUSED: the restored source does not pass")
        return 2
    print("RESTORED: the unmutated owner passes again")
    return 0


if __name__ == "__main__":
    sys.exit(main())
