#!/usr/bin/env python3
"""pad_decode.py — decode a PSXPORT pad-replay (.pad) into a human button timeline and/or an
SBS_KEYS string, and (inverse) build a .pad from a timeline. This is how you READ a recorded route
(replays/*.pad are 2-byte active-low PSX pad masks per frame) to learn its input, and how you
GENERATE a scripted route for headless SBS driving.

Why: the replays ARE working input sequences to reach a scene. Decoding one shows exactly what it
presses (e.g. hut-entry = boot taps, then walk RIGHT to the door, then hold UP to enter). Combined
with the movement calibration in docs/driving-the-game.md ("Driving by position feedback"), you can
author your own routes to any reachable target without live capture.

USAGE:
  tools/pad_decode.py <file.pad>              # print the button-press timeline (collapsed runs)
  tools/pad_decode.py <file.pad> --keys       # print an SBS_KEYS="FROM-TO:BTN,..." string
  tools/pad_decode.py --keys-from "220-254:right,255-435:up" --out route.pad --frames 500
                                              # build a .pad from an SBS_KEYS-style spec
  tools/pad_decode.py --keys-from "1700-1707:cross" --base <recorded.pad> --frames 2200
                                              # EXTEND a recorded route: the base's frames
                                              # are kept byte-for-byte and the spec's
                                              # frame numbers are absolute

A combined press is emitted as one range PER BUTTON (`233-236:up,233-236:circle`), because
--keys-from ANDs overlapping ranges. `--keys` therefore round-trips losslessly; it used to drop
every combined press silently, which is why routes rebuilt from it desynced.

Pad bit layout (active-low: a PRESSED button CLEARS its bit; neutral frame = 0xFFFF). LITTLE-ENDIAN
uint16 per frame (verified against the replay library — LE yields ~76% neutral frames, BE ~0%)."""
import os, sys, struct

# The complete SCPH digital-pad word lives in the framework, with the .pad format that uses it:
# external/psxport/tools/psx_pad.py. It was duplicated here, and the shoulder and stick bits were
# missing from this copy until a round-trip over the whole replay library hit mask 0xFEFF (L2 held)
# in long-session-many-bugs.pad and could not name it. An incomplete copy of a bit table makes a
# rebuilt route drop input silently, which is why there is now exactly one.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "external", "psxport", "tools"))
from psx_pad import BITS_TO_NAME as BTN, PSX_BUTTON_BITS as NAME2BIT  # noqa: E402


def decode(path):
    data = open(path, "rb").read()
    n = len(data) // 2
    return [struct.unpack_from("<H", data, i * 2)[0] for i in range(n)]


def timeline(masks):
    """Collapse frame masks into (from, to, buttons) runs, skipping neutral (0xFFFF) stretches."""
    runs, prev, start = [], None, 0
    for f, m in enumerate(list(masks) + [None]):
        if m != prev:
            if prev is not None and prev != 0xFFFF:
                btns = "+".join(b for bit, b in BTN.items() if not (prev & bit))
                runs.append((start, f - 1, btns or hex(prev)))
            start, prev = f, m
    return runs


def keys_spec(runs):
    """Render timeline runs as an SBS_KEYS spec. A combined press becomes one range per button, so
    build_pad's AND-overlap reproduces it exactly. An undecodable mask is REFUSED, never dropped —
    a spec that silently omits input produces a route that desyncs and looks like a game bug."""
    parts = []
    for first, last, buttons in runs:
        if buttons.startswith("0x"):
            raise SystemExit(f"f{first}-{last}: mask {buttons} has no known button bits; "
                             f"refusing to emit a spec that would silently drop it")
        for name in buttons.split("+"):
            parts.append(f"{first}-{last}:{name}")
    return ",".join(parts)


def build_pad(spec, nframes, base=b""):
    """spec = 'FROM-TO:BTN,...' -> bytes of nframes little-endian masks (0xFFFF neutral, bit cleared
    for each active button in its range). Multiple ranges may overlap (ANDs the bits)."""
    prefix = [struct.unpack_from("<H", base, i * 2)[0] for i in range(len(base) // 2)]
    if len(prefix) > nframes:
        raise SystemExit(f"--base has {len(prefix)} frames, more than the requested {nframes}")
    masks = prefix + [0xFFFF] * (nframes - len(prefix))
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        rng, name = part.split(":")
        a, b = rng.split("-")
        bit = NAME2BIT.get(name.strip())
        if bit is None:
            raise SystemExit(f"unknown button: {name}")
        for f in range(int(a), min(int(b), nframes - 1) + 1):
            masks[f] &= ~bit & 0xFFFF
    return b"".join(struct.pack("<H", m) for m in masks)


def main():
    a = sys.argv[1:]
    if "--keys-from" in a:
        spec = a[a.index("--keys-from") + 1]
        out = a[a.index("--out") + 1] if "--out" in a else "route.pad"
        nframes = int(a[a.index("--frames") + 1]) if "--frames" in a else 600
        base_path = a[a.index("--base") + 1] if "--base" in a else None
        base = open(base_path, "rb").read() if base_path else b""
        open(out, "wb").write(build_pad(spec, nframes, base))
        kept = len(base) // 2
        origin = f", keeping {kept} frame(s) of {base_path} byte-for-byte" if kept else ""
        print(f"wrote {out}: {nframes} frames from spec {spec!r}{origin}")
        return
    if not a or a[0].startswith("--"):
        print(__doc__)
        return
    path = a[0]
    runs = timeline(decode(path))
    if "--keys" in a:
        print(keys_spec(runs))
    else:
        print(f"{path}: {len(decode(path))} frames")
        for f, t, b in runs:
            print(f"  f{f}-{t}: {b}")


if __name__ == "__main__":
    main()
