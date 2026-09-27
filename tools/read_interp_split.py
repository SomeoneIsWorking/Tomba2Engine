#!/usr/bin/env python3
"""read_interp_split.py — print the product's OWN real/interp/total split for the current run.

WHY A SEPARATE FILE. The interpolation claim needs a number the oracle cannot supply: the oracle
compares GUEST STATE, and an interpolated in-between never advances guest state, so 272 decisive
comparisons at 0 unequal are consistent with a leg that interpolated nothing at all. The product
publishes the split itself (`dbg_server.cpp` answers `frame` with `frame=N interp=I total=T`), and
`live_play.py` has a `frames()` that reads it -- but nothing printed it, so the one number that
decides whether the lerp scope is real was being measured and thrown away.

This is therefore a THIN reader over the existing client, not a second transport: it imports
`live_play.ask`, so the two refusals that tool already makes (a server timeout and an empty reply,
either of which reads as zero if taken as data) still apply.

It prints the raw reply alongside the verdict, so the verdict is auditable rather than asserted.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from live_play import Refusal, ask  # noqa: E402  the one transport, and its two refusals


def verdict(real: int, interp: int, total: int) -> str:
    if total <= 0:
        return "NO VERDICT: total=0, so the counters say nothing about this leg"
    if interp <= 0:
        return (
            f"NOT INTERPOLATED: total={total} presented, interp=0. The fps60 mode is enabled but "
            f"nothing was reconstructed, so an oracle 0-unequal on this leg is VACUOUS for the "
            f"interpolation claim."
        )
    if real <= 0:
        return f"NOT A SIMULATION AT ALL: total={total} with real=0"
    return (
        f"INTERPOLATED: {interp} of {total} presented frames were reconstructed "
        f"({100.0 * interp / total:.1f}%), over {real} real ones"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, required=True)
    args = parser.parse_args()

    import live_play

    client = live_play.LiveClient(args.port)
    try:
        reply = ask(client, "frame")
    except Refusal as refusal:
        # A refusal here is a fact about the run, and reporting it as zero interp would be the
        # exact failure this file exists to prevent.
        print(f"{refusal}", file=sys.stderr)
        return 2
    finally:
        client.close()

    print(f"[interp] the endpoint's own reply: {reply.strip()!r}")
    found = {}
    for token in reply.split():
        for key in ("frame", "interp", "total"):
            if token.startswith(key + "="):
                found[key] = int(token.split("=", 1)[1])
    if "total" not in found or "interp" not in found:
        print(
            f"[interp] REFUSING: the reply carries no interp=/total= counters, so this binary "
            f"cannot answer a presentation-cadence question: {reply.strip()!r}"
        )
        return 2
    line = verdict(found.get("frame", 0), found["interp"], found["total"])
    print(f"[interp] {line}")
    return 0 if found["interp"] > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
