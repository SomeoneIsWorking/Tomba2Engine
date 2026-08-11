#!/usr/bin/env python3
"""producer_frontier.py — emit the OWNERSHIP FRONTIER for external/psxport/tools/producer_class.py.

producer_class.py classifies guest graphics producers and reports each shared dependency that carries
per-vertex lighting/colour ops as a judgement call. Whether that judgement is still OUTSTANDING depends
on something the framework tool cannot know: does a native owner already exist? It takes that answer as
a `--frontier` file of bare addresses, and this generates it from THIS repo's authoritative index.

WHY THIS EXISTS RATHER THAN PASSING docs/code-map.md DIRECTLY. Both naive shortcuts were tried and both
were wrong, in opposite directions:
  * a HAND-TYPED frontier reported 0x80085480 (Math::rotmat) and 0x80027A4C (Render::fxSpriteRender) as
    outstanding blockers when both are native — a 3x OVERstatement of the gate;
  * SCRAPING docs/code-map.md for hex tokens swallowed 0x80027768, which appears there only in the
    address column of a `todo` port-map row, and called it ported — an UNDERstatement, and the worse
    direction, because it retires a judgement call nobody made.
"Mentioned" and "owned" are indistinguishable in prose, so ownership must come from the tool that
decides it: `codemap.py --addr`, which answers OWNED vs NO NATIVE OWNER and declares its own blind
spots. This script is that query in bulk, and it prints the UNOWNED set too — a frontier generator that
only reported what it found would hide how much it did not.

USAGE
    python3 tools/producer_frontier.py <addr> [<addr> ...]        > scratch/frontier.txt
    python3 tools/producer_frontier.py --from-json <verdicts.json> > scratch/frontier.txt

`--from-json` reads producer_class.py's `--json` output and takes every `blocked_on` + `resolved_deps`
address, so the two tools chain without a hand-maintained list in between:

    python3 external/psxport/tools/producer_class.py --repo . classify --file addrs.txt --json v.json
    python3 tools/producer_frontier.py --from-json v.json > scratch/frontier.txt
    python3 external/psxport/tools/producer_class.py --repo . classify --file addrs.txt \
        --frontier scratch/frontier.txt
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UNOWNED_MARK = 'NO native owner'
BEHAVIOURAL = os.path.join(REPO, 'docs', 'producers', 'behavioural-owners.txt')


def norm(a: str) -> str:
    a = a.strip().upper()
    if a.startswith('0X'):
        a = a[2:]
    return a.zfill(8)


def owner_of(addr: str) -> tuple:
    """(is_owned, first-line detail) for one address, per tools/codemap.py --addr."""
    r = subprocess.run([sys.executable, os.path.join(REPO, 'tools', 'codemap.py'),
                        '--addr', '0x' + addr],
                       capture_output=True, text=True, cwd=REPO)
    if r.returncode != 0:
        raise SystemExit(f"codemap.py --addr 0x{addr} failed (exit {r.returncode}): "
                         f"{r.stderr.strip()[:300]}\nRefusing to emit a frontier from a broken query — "
                         f"an address wrongly reported UNOWNED overstates the work, and one wrongly "
                         f"reported OWNED retires a judgement nobody made.")
    out = r.stdout
    if UNOWNED_MARK in out:
        return False, 'no native owner'
    m = re.search(r'([A-Za-z_][A-Za-z0-9_]*::[A-Za-z0-9_]+)', out)
    return True, (m.group(1) if m else out.strip().split('\n')[0][:80])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('addrs', nargs='*')
    ap.add_argument('--from-json', help="producer_class.py --json output; takes its shared deps")
    args = ap.parse_args()

    addrs = [norm(a) for a in args.addrs]
    if args.from_json:
        with open(args.from_json) as f:
            d = json.load(f)
        for v in d.get('verdicts', []):
            for key in ('blocked_on', 'resolved_deps'):
                for b in v.get(key, []):
                    addrs.append(norm(b['addr']))
    addrs = sorted(set(addrs))
    if not addrs:
        print("no addresses given (pass them, or --from-json) — emitted NOTHING, which as a frontier "
              "would report every shared dependency as outstanding.", file=sys.stderr)
        return 2

    # Behavioural owners: writers owned by a native that records no guest address, so codemap cannot
    # see them (its declared blind spot #3). Cited per row in the file; see its header for the bar.
    behavioural: dict = {}
    if os.path.isfile(BEHAVIOURAL):
        with open(BEHAVIOURAL) as f:
            for line in f:
                t = line.split('#')[0].strip()
                if t:
                    behavioural[norm(t)] = True

    owned, unowned, behav = [], [], []
    for a in addrs:
        ok, detail = owner_of(a)
        if ok:
            owned.append((a, detail))
        elif a in behavioural:
            behav.append((a, 'behavioural owner (see docs/producers/behavioural-owners.txt)'))
        else:
            unowned.append((a, detail))

    print(f"# ownership frontier for producer_class.py --frontier")
    print(f"# generated by tools/producer_frontier.py from tools/codemap.py --addr (authoritative)")
    print(f"# queried {len(addrs)} address(es): {len(owned)} OWNED by address, {len(behav)} owned "
          f"BEHAVIOURALLY, {len(unowned)} UNOWNED")
    for a, detail in owned + behav:
        print(f"0x{a}   # {detail}")
    # The denominator, on stderr so it cannot pollute the frontier: a generator that printed only its
    # hits would read as "everything is ported" when it is the unowned list that is the actual work.
    print(f"\n[frontier] {len(owned)} owned by address, {len(behav)} owned behaviourally, "
          f"{len(unowned)} UNOWNED (these remain judgement calls):", file=sys.stderr)
    for a, _ in unowned:
        print(f"[frontier]   0x{a}  no native owner", file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
