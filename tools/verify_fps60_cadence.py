#!/usr/bin/env python3
"""Does Tomba! 2's 60fps feature actually PRESENT at 60 Hz, with guest behaviour unchanged?

The evidence so far is an ORACLE result: 34/34 state checkpoints byte-identical with the feature on and
off, which establishes the harder half — that interpolation leaves guest state unperturbed. It says nothing
about whether the interpolated frames reach the screen, and a product that computed midpoints and then
presented at 30 Hz would pass that oracle unchanged.

So this measures the other half, over the live control channel, and measures it with a CONTROL LEG,
because neither number means anything alone.

## The denominator is guest PROGRESS, not a tick counter

This title's tooling has no guest logic-tick symbol, and inventing one would be a guess. So the
denominator is the player's own displacement over a held input — and that is a BETTER denominator than a
tick counter, not a worse one, for two reasons:

1. **It must be identical between the legs.** Both legs hold the same input for the same presented-frame
   budget, and interpolation must not change where the player ends up. So if the two legs disagree on
   displacement, that is a FIDELITY finding in its own right, and the tool reports it as a failure rather
   than quietly dividing by it.
2. **It is what "the feature does not change the game" means operationally.** A cadence claim that also
   moved the player would be worthless.

## The numerator, and the trap in it

Presented frames per unit progress, using the endpoint's **total** present counter. This is the second
version of this measurement on this workspace and the first one was wrong for a reason worth repeating:
`frame` counts REAL presents only, because an interpolated in-between reaches the screen without
advancing it. On Spyro 1 a live run with the feature on read **1.006** real frames per guest update while
emitting 3,915 in-betweens — a confident, plausible, wrong answer. psxport `7e3ae28f` added the missing
counter and made the endpoint answer `frame=<real> interp=<in-between> total=<sum>`.

## What this does NOT claim

- Not that the interpolated geometry is CORRECT, only that it is presented and that guest behaviour is
  unchanged. Correctness is a per-object question against the captured endpoint pair.
- Not a wall-clock rate: agent runs are unpaced, so this is presentations per unit of progress.
- Not a pacing or audio claim.

## Why it drives the real product

Boot, logos and menus do not establish frame cadence, and neither does a static trace. This launches the
shipping product headless, walks the documented route into gameplay, holds real input over the endpoint,
and reads the counters and the presented pixels from the running process. A route it cannot reach is a
REFUSAL, not a pass.
"""

from __future__ import annotations

import argparse
import filecmp
import json
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "external" / "psxport" / "tools"))

import live_play  # noqa: E402

SHIPPING = ROOT / "tools" / "shipping_settings.ini"
CONTROL = ROOT / "tools" / "fps60_control_settings.ini"
SHOTS = ROOT / "scratch" / "fps60_cadence"


def progress(session: live_play.Session) -> int:
    """Total displacement in 16.16 units, so two samples can be subtracted without a rounding tail."""
    return sum(abs(value) for value in session.player_position())


def cadence(session: live_play.Session, *, windows: int, seconds: float) -> dict:
    """Presented frames per unit of guest progress, over several held-input windows.

    Input is held ACROSS all windows, not per window: releasing and re-pressing changes the game state
    under the measurement, which is what made an earlier wall-clock version of this tool report per-window
    rates of 1.00, 1.62 and 5.96 and correctly refuse to call any of them a cadence.
    """
    client = session.client
    client.press("right")
    try:
        samples: list[dict] = []
        for index in range(windows):
            before_frames = session.frames()
            before_progress = progress(session)
            started = time.monotonic()
            while time.monotonic() - started < seconds:
                time.sleep(0.005)
            after_frames = session.frames()
            moved = progress(session) - before_progress
            total = after_frames["total"] - before_frames["total"]
            samples.append({
                "window": index,
                "presented_frames": total,
                "real_presents": after_frames["frame"] - before_frames["frame"],
                "interpolated_presents": after_frames["interp"] - before_frames["interp"],
                "progress": moved,
                "frames_per_progress": round(total / moved, 6) if moved else None,
            })
    finally:
        client.release("right")
    rates = [s["frames_per_progress"] for s in samples if s["frames_per_progress"] is not None]
    if not rates:
        raise SystemExit(
            "REFUSED: the player did not move in any window, so there is no guest progress to measure a "
            "presentation rate against"
        )
    return {
        "windows": samples,
        "frames_per_progress_min": min(rates),
        "frames_per_progress_max": max(rates),
        "frames_per_progress": sum(rates) / len(rates),
    }


def leg(name: str, settings: Path, port: int, *, windows: int, seconds: float, shots: int,
        budget: int) -> dict:
    log = ROOT / "scratch" / "logs" / f"fps60-{name}.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    product = live_play.launch(port, ROOT / "build" / "bin" / "tomba2_port", log, settings=settings)
    try:
        client = live_play.connect(port, 180.0, log)
        session = live_play.Session(client)
        # The budget is in PRESENTED frames, so with the feature on a leg gets half the guest
        # updates a 30 Hz leg does over the same number. Sizing it for the 30 Hz case refuses
        # the shipped leg before it measures anything, which is a budget error and not a finding.
        session.drive_to_gameplay(budget, 1800.0, 60)
        knobs = live_play.effective_configuration(session.client)
        fps60 = knobs["knobs"].get("PSXPORT_FPS60")
        if fps60 is None:
            raise SystemExit(
                "REFUSED: the product reports no PSXPORT_FPS60 cvar, so this run cannot say whether it "
                "was testing the feature on or off"
            )
        result = {
            "leg": name,
            "settings": str(settings.relative_to(ROOT)),
            "PSXPORT_FPS60": fps60,
            "PSXPORT_FPS60_layer": knobs["layers"].get("PSXPORT_FPS60"),
            "cadence": cadence(session, windows=windows, seconds=seconds),
        }
        paths = []
        for index in range(shots):
            before = session.frames()
            deadline = time.monotonic() + 60.0
            while session.frames()["total"] < before["total"] + 1:
                if time.monotonic() > deadline:
                    raise SystemExit(f"REFUSED: capture {index} never saw the next presentation")
                time.sleep(0.001)
            SHOTS.mkdir(parents=True, exist_ok=True)
            path = SHOTS / f"{name}_{index:03d}.png"
            if "->" not in session.client.send(f"shot {path}"):
                raise SystemExit(f"REFUSED: the endpoint did not capture a presented frame at {path}")
            paths.append(path)
        result["motion"] = {
            "captures": len(paths),
            "pairs": len(paths) - 1,
            "identical_consecutive_pairs": sum(
                1 for i in range(len(paths) - 1) if filecmp.cmp(paths[i], paths[i + 1], shallow=False)
            ),
        }
        return result
    finally:
        product.terminate()
        try:
            product.wait(timeout=30)
        except subprocess.TimeoutExpired:  # pragma: no cover - only on a wedged product
            product.kill()
            product.wait(timeout=30)


def verdict(shipped: dict, control: dict) -> tuple[int, list[str]]:
    lines: list[str] = []
    failures = 0
    for result in (shipped, control):
        rate = result["cadence"]
        lines.append(
            f"[cadence] {result['leg']:8s} PSXPORT_FPS60={result['PSXPORT_FPS60']:5s} "
            f"[{result['PSXPORT_FPS60_layer']}]: {rate['frames_per_progress']:.6f} presented frames per "
            f"unit of progress, per window "
            f"{[w['frames_per_progress'] for w in rate['windows']]}"
        )
    if shipped["PSXPORT_FPS60"] != "true":
        lines.append("[cadence] REFUSED: the shipped leg did not have the feature on")
        failures += 1
    if control["PSXPORT_FPS60"] != "false":
        lines.append("[cadence] REFUSED: the control leg did not have the feature off, so there is no control")
        failures += 1
    # FIDELITY FIRST. The denominator is guest behaviour, so the two legs must agree on it or the
    # cadence ratio is dividing by two different things and means nothing.
    ship_progress = sum(w["progress"] for w in shipped["cadence"]["windows"])
    ctrl_progress = sum(w["progress"] for w in control["cadence"]["windows"])
    drift = abs(ship_progress - ctrl_progress) / max(ctrl_progress, 1)
    lines.append(f"[fidelity] guest progress over the windows: shipped {ship_progress} vs control "
                 f"{ctrl_progress} ({drift * 100:.2f}% apart)")
    if drift > 0.05:
        lines.append(
            f"[fidelity] the two legs moved the player {drift * 100:.2f}% differently under the same "
            f"input, so the feature is CHANGING GUEST BEHAVIOUR and the cadence ratio below divides by "
            f"two different quantities"
        )
        failures += 1
    if failures:
        return 1, lines
    gain = shipped["cadence"]["frames_per_progress"] / control["cadence"]["frames_per_progress"]
    lines.append(f"[cadence] the feature presents {gain:.4f}x as many frames per unit of progress")
    if gain < 1.8 or gain > 2.2:
        lines.append(
            f"[cadence] that is not ~2x, which is what doubling a 30 Hz title's presentation rate looks "
            f"like. Either the feature is not presenting the in-betweens, or the control is not what it "
            f"claims to be"
        )
        failures += 1
    for result in (shipped, control):
        motion = result["motion"]
        lines.append(f"[motion] {result['leg']:8s}: {motion['identical_consecutive_pairs']} of "
                     f"{motion['pairs']} consecutive presented-frame pairs byte-identical")
    if shipped["motion"]["identical_consecutive_pairs"]:
        lines.append(
            "[motion] the shipped leg presented byte-identical consecutive frames, so its frame NUMBERS "
            "advance while its PICTURES do not"
        )
        failures += 1
    return (1 if failures else 0), lines


def _leg(name: str, feature: str, layer: str, rate: float, progress: int,
         identical: int, pairs: int) -> dict:
    """One leg's measured shape, so the suite can state a case instead of hand-writing nested dicts."""
    return {
        "leg": name,
        "PSXPORT_FPS60": feature,
        "PSXPORT_FPS60_layer": layer,
        "cadence": {
            "frames_per_progress": rate,
            "windows": [
                {"frames_per_progress": rate, "progress": progress // 2},
                {"frames_per_progress": rate, "progress": progress - progress // 2},
            ],
        },
        "motion": {"identical_consecutive_pairs": identical, "pairs": pairs},
    }


def selftest() -> int:
    """`verdict` on fixtures, driving NOTHING — and the suite exists because this decision is the claim.

    Every assertion this tool makes is a line out of `verdict`, and the two that matter most are both
    REFUSALS: that the control leg really had the feature off, and that the two legs moved the guest the
    same amount. A `verdict` that could only say "pass" would be untestable in the one direction that
    matters, so the suite requires all three answers — a clean ~2x pass, and a refusal for each of the two
    ways a run can look fine while proving nothing.

    The gain window is 1.8..2.2, and both edges are checked, because "not ~2x" is the difference between
    "the in-betweens are not being presented" and "the control is not what it claims to be" — two
    opposite root causes that a single interior case would never separate.
    """
    total = 0
    failures = 0

    def expect(name: str, code: int, lines: list[str], needle: str) -> None:
        nonlocal total, failures
        total += 1
        ok = code == 0 and any(needle in line for line in lines)
        if not ok:
            failures += 1
        print(f"  {'ok  ' if ok else 'FAIL'} {name}: exit {code}"
              + ("" if code == 0 else f", expected 0")
              + (f"; {'found' if any(needle in l for l in lines) else 'MISSING'} {needle!r}"))

    # 1. The pass: ~2x, matching denominators, no identical consecutive frames. The rates are the two
    #    MEASURED Spyro 1 legs (2.0165 vs 1.0061 presented frames per unit of progress) and the expected
    #    gain string is COMPUTED from them rather than written out, so a wrong expectation here is
    #    impossible by construction — a hardcoded "2.0043x" would be a number that silently rots when
    #    the division is done slightly differently, and would then read as the tool being wrong.
    ship_rate, ctrl_rate = 2.0165, 1.0061
    expect("clean ~2x pass",
           *verdict(_leg("shipped", "true", "f2", ship_rate, 1516, 0, 2208),
                    _leg("control", "false", "f0", ctrl_rate, 1516, 2207, 2208)),
           needle=f"{ship_rate / ctrl_rate:.4f}x")

    # 2. The control leg is the whole point: if it was not off, the ratio divides by nothing.
    code, lines = verdict(_leg("shipped", "true", "f2", 2.0, 1000, 0, 900),
                          _leg("control", "true", "f2", 1.0, 1000, 0, 900))
    total += 1
    if code == 0 or not any("control leg did not have the feature off" in l for l in lines):
        failures += 1
    print(f"  {'ok  ' if code else 'FAIL'} control leg with the feature ON is refused: exit {code}")

    # 3. Denominator drift: the cadence ratio would divide two different quantities.
    code, lines = verdict(_leg("shipped", "true", "f2", 2.0, 1000, 0, 900),
                          _leg("control", "false", "f0", 1.0, 1400, 0, 900))
    total += 1
    if code == 0 or not any("CHANGING GUEST BEHAVIOUR" in l for l in lines):
        failures += 1
    print(f"  {'ok  ' if code else 'FAIL'} legs that moved the guest differently are refused: exit {code}")

    # 4. Byte-identical consecutive frames: frame NUMBERS advance while PICTURES do not.
    code, lines = verdict(_leg("shipped", "true", "f2", 2.0, 1000, 7, 900),
                          _leg("control", "false", "f0", 1.0, 1000, 0, 900))
    total += 1
    if code == 0 or not any("byte-identical consecutive frames" in l for l in lines):
        failures += 1
    print(f"  {'ok  ' if code else 'FAIL'} identical consecutive frames are refused: exit {code}")

    # 5. Both edges of the ~2x window, which are opposite root causes.
    for rate, name in ((1.05, "gain below the window (in-betweens not presented)"),
                       (2.90, "gain above the window (the control is not the control)")):
        code, lines = verdict(_leg("shipped", "true", "f2", rate, 1000, 0, 900),
                              _leg("control", "false", "f0", 1.0, 1000, 0, 900))
        total += 1
        if code == 0 or not any("not ~2x" in l for l in lines):
            failures += 1
        print(f"  {'ok  ' if code else 'FAIL'} {name}: exit {code}")

    if failures:
        print(f"  selftest: {failures} of {total} case(s) FAILED")
        return 1
    print(f"  selftest: {total}/{total} cases behaved as required. Three of them are refusals, which is "
          "the point: a verdict that could only report success would not be able to say that a control "
          "leg was not a control, that the two legs moved the guest differently, or that the shipped leg "
          "presented the same picture twice in a row.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=5983)
    parser.add_argument("--windows", type=int, default=3)
    parser.add_argument("--seconds", type=float, default=5.0)
    parser.add_argument("--shots", type=int, default=6)
    parser.add_argument("--budget", type=int, default=60000,
                        help="presented frames allowed to reach gameplay (default 60000)")
    parser.add_argument("--leg", choices=("shipped", "control", "both"), default="both")
    parser.add_argument("--out", type=Path, default=ROOT / "scratch" / "fps60_cadence" / "report.json")
    parser.add_argument("--selftest", action="store_true",
                        help="run the shipped-vs-control verdict against fixtures; launches no product "
                             "and reads no game data")
    args = parser.parse_args(argv)
    # FIRST THING, before any leg: a selftest that launched a product would be the worst possible
    # answer to "does this instrument check itself", so the fixtures path returns before the first
    # `leg()` call and before the report file is written.
    if args.selftest:
        return selftest()

    results: dict[str, dict] = {}
    try:
        if args.leg in ("shipped", "both"):
            results["shipped"] = leg("shipped", SHIPPING, args.port, windows=args.windows,
                                     seconds=args.seconds, shots=args.shots, budget=args.budget)
        if args.leg in ("control", "both"):
            results["control"] = leg("control", CONTROL, args.port + 1, windows=args.windows,
                                     seconds=args.seconds, shots=args.shots, budget=args.budget)
    finally:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(results, indent=2, default=str))
    print(f"[report] {args.out}")
    if "shipped" in results and "control" in results:
        status, lines = verdict(results["shipped"], results["control"])
        for line in lines:
            print(line)
        return status
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
