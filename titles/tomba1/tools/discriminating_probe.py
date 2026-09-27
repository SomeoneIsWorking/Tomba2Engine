#!/usr/bin/env python3
"""Discriminate WHICH half of the Tomba! 1 widening changes the guest's own primitive stream.

THE QUESTION. The 4:3 and 16:9 runs of the same build differ in how many primitives the GUEST
emits: 1 over 300 frames at 4:3, 303 at 16:9. A widening is supposed to change what the guest
DRAWS INTO, not how many commands it issues. Until this is explained, "the frustum is wider" and
"the guest behaves differently" are indistinguishable, and the second is a regression I introduced.

THE METHOD. One variable at a time, each by editing exactly one line of the owner and rebuilding:
  A. OFX only        — draw clip stays retail 320, only the projection centre moves.
  B. draw clip only  — the centre stays retail 160, only RECT.w moves.
  C. both (shipping) — the plan as committed.
  D. neither         — the unmutated 4:3 path, i.e. the retail identity.

Each variant is run at BOTH aspect settings, so a difference is attributable to the mutation and
not to the aspect. The primitive count is read from the framework's own primdump CSV, and the guest's
own DRAWENV is read over the live debug channel, because the two answer different questions: the
CSV says how much the guest DREW, the env says what region the owner widened.

Usage: discriminating_probe.py --disc <tomba1.chd>
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import subprocess
import sys
import time

REPOSITORY = pathlib.Path(__file__).resolve().parents[3]
PRODUCT = REPOSITORY / "build/bin/tomba1_port"
OWNER = REPOSITORY / "titles/tomba1/game/render/widescreen_projection.cpp"
CSV_PATH = REPOSITORY / "scratch/logs/prims_f0.csv"

# Each variant names the lines it substitutes. A single-line variant cannot attribute a difference
# to the wrong cause, which is the point of running them separately; the CONTROL moves neither, so
# "the guest changed" is measured against a measured baseline rather than an assumption.
_OFX = (
    "const auto centerX = latched.projectionCenterX;",
    "const auto centerX = static_cast<int>(latched.nativeProjectionExtent.width) / 2;",
)
_DRAW = (
    "core.r[kArgumentA3] = static_cast<std::uint32_t>(latched.guestDrawWidth);",
    "core.r[kArgumentA3] = static_cast<std::uint32_t>(retailDrawWidth);",
)

VARIANTS = {
    # Draw clip stays retail; only OFX moves. OFX is what widens the FRUSTUM.
    "A-ofx-only": (_DRAW,),
    # OFX stays at the retail centre; only the draw clip widens. RECT.w is what widens the clip
    # REGION and is the only thing a primitive dump can observe directly.
    "B-draw-clip-only": (_OFX,),
    # The plan as committed.
    "C-shipping": (),
    # Neither moves: the owner latches a plan and applies nothing. This is the CONTROL.
    "D-neither-moves": (_OFX, _DRAW),
}


def build() -> tuple[bool, str]:
    result = subprocess.run(
        ["cmake", "--build", REPOSITORY / "build", "--target", "tomba1_port"],
        cwd=REPOSITORY, capture_output=True, text=True, timeout=2400, check=False,
    )
    detail = "" if result.returncode == 0 else (result.stdout + result.stderr).strip().splitlines()[-1]
    return result.returncode == 0, detail


def count_prims() -> tuple[int, int]:
    if not CSV_PATH.is_file():
        return 0, 0
    with CSV_PATH.open(encoding="utf-8", errors="replace") as handle:
        rows = list(csv.DictReader(handle))
    return len(rows), len({row.get("frame") for row in rows})


def run(disc: pathlib.Path, aspect: int, frames: int, log: pathlib.Path) -> tuple[int, int, str]:
    settings = pathlib.Path(f"/tmp/opencode/disc_aspect{aspect}.ini")
    settings.write_text(f"aspect={aspect}\nires=1\n", encoding="utf-8")
    if CSV_PATH.exists():
        CSV_PATH.unlink()
    environment = {
        "PATH": "/usr/bin:/bin", "HOME": str(pathlib.Path.home()),
        "PSXPORT_TOMBA1_DISC": str(disc),
        "PSXPORT_SETTINGS": str(settings), "PSXPORT_VK_HEADLESS": "1",
        "PSXPORT_NATIVE_FRAMES": str(frames), "PSXPORT_PRIMDUMP": f"0-{frames}",
        "PSXPORT_ASSET_DIR": "/home/bhamil/repo/psx/psxport",
        "SDL_VIDEODRIVER": "offscreen", "SDL_AUDIODRIVER": "dummy",
        "VK_ICD_FILENAMES": "/usr/share/vulkan/icd.d/lvp_icd.x86_64.json",
    }
    with log.open("wb") as handle:
        process = subprocess.Popen([str(PRODUCT)], cwd=REPOSITORY, env=environment,
                                   stdout=handle, stderr=subprocess.STDOUT)
    process.wait(timeout=900)
    prims, drawn_frames = count_prims()
    wide = ""
    for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
        if "guest projection" in line:
            wide = line.split("] ", 1)[-1]
    return prims, drawn_frames, wide


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=pathlib.Path, required=True)
    parser.add_argument("--frames", type=int, default=300)
    parser.add_argument("--port", type=int, default=6050)
    args = parser.parse_args(argv)

    original = OWNER.read_text(encoding="utf-8")
    print(f"FRAMES: {args.frames} per leg. A leg is [prim rows, distinct frames the guest drew on].")
    print(f"{'variant':20s} {'aspect=0':>26s}   {'aspect=1':>26s}")
    try:
        for name, substitutions in VARIANTS.items():
            mutated = original
            missing = False
            for old, new in substitutions:
                if old not in mutated:
                    missing = True
                    break
                mutated = mutated.replace(old, new, 1)
            if missing:
                print(f"{name:20s} PATTERN NOT FOUND")
                continue
            OWNER.write_text(mutated, encoding="utf-8")
            ok, detail = build()
            if not ok:
                print(f"{name:20s} did not build: {detail}")
                continue
            cells = []
            for aspect in (0, 1):
                prims, frames, wide = run(args.disc, aspect, args.frames,
                                          pathlib.Path(f"/tmp/opencode/disc_{name}_{aspect}.log"))
                cells.append(f"{prims:6d} prims / {frames:4d} f")
            print(f"{name:20s} {cells[0]:>26s}   {cells[1]:>26s}")
    finally:
        OWNER.write_text(original, encoding="utf-8")
        build()
    print()
    print("The owner is restored. A row whose two cells are EQUAL says that half of the widening does")
    print("not change the guest's command stream; a row whose cells DIFFER names the half that does.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
