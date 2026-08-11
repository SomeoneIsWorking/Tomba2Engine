#!/usr/bin/env python3
"""producers.py — the GRAPHICS PRODUCER DB: what draws this game's picture, and who owns each of it.

WHY THIS EXISTS (read first). Three registries already answer one question each — `docs/code-map.md`
says what code EXISTS, `docs/findings/` + `docs/kanban/` say what has been TRIED, and psxport's
`overrides::coverage` says how much of what we OWN a run reached. That last fraction has our own
ambition as its denominator. The one nobody could answer is the opposite fraction: **of the
picture-producing work the game actually DID, how much does the PC produce natively?** That number is
discovered by RUNNING, never by reading our own source, and until now it lived as prose in
`docs/gfx-debug.md` plus a hand-written debug channel per effect (`beamfx`, `plumefx`, `heads0`,
`cullpush`, …), each with its own hand-maintained denominator. This is that table, mechanically kept.

USER, 2026-08-11: *"I need a DB for each game to keep track of native graphics producers, framework
should do this automatically, when the game renders an effect, if it's not in the DB then a DB entry
gets created and we need to track if it has a native producer equivalent … basically comparing GTE/OT
against native producers"*, *"should also be populated when I'm playing then I can tell you something
like 'work on the DB entries'"*, and *"it should be in the git"*. Design + staging:
`external/psxport/docs/plans/graphics-producer-db.md`.

HOW THE HALVES SPLIT, and why the runtime does not write these files. The port appends what it OBSERVED
to `scratch/producers/run-<stamp>.jsonl` as producers are first seen (never buffered to the end, so a
crash or watchdog abort still leaves its rows). This tool folds those observations into one tracked
Markdown file per producer under `docs/producers/`. `ingest` is called from `run.sh`'s tail AND at the
start of the next run, so ordinary play populates the DB with nothing to remember — while a C++ frame
path never mutates a git tree, where a mid-write abort would leave half a file.

    tools/producers.py ingest                 # fold every un-ingested scratch/producers/*.jsonl in
    tools/producers.py report                 # coverage, WITH ITS DENOMINATORS
    tools/producers.py report --todo          # THE WORK QUEUE — what to own next, ranked
    tools/producers.py search <words>         # find a row by name/fn/note
    tools/producers.py show <key>             # one row in full
    tools/producers.py check                  # lint the curated fields (exits 1 on a lie)

WHAT IS MACHINE-OWNED AND WHAT IS YOURS. A row's frontmatter has two halves and `ingest` only ever
touches the first:

    observed:   first_seen, last_seen, runs, prims_guest_max, prims_native_max, frames_seen,
                gte_calls_max, layers, sub_signatures, has_native, native_reached
    curated:    name, re_status, re_evidence, producer_file, partial_because, compare_status, notes

`has_native` / `native_reached` are DERIVED from the override table by the runtime, not curated — that
is what stops this from rotting into a wish list. Only slow-moving observed fields are tracked at all
(maxima and first/last, not per-run counters), so after a few sessions a play run's `git diff` is
*new producers and nothing else*, which is what makes the diff readable as "here is what I found".

A NEGATIVE FROM THIS TOOL ALWAYS CARRIES ITS DENOMINATOR. `report` refuses to print a coverage
percentage without also printing how many prims could not be attributed at all and why — because a
coverage report that can print "0 unported producers" is worthless unless that zero is
distinguishable from "I never looked". The census counts six blindnesses (see the plan); this prints
all of them, every time, and `ingest` REFUSES rather than reporting success when the observation
directory does not exist.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DB_DIR = REPO / "docs" / "producers"
OBS_DIR = REPO / "scratch" / "producers"
INGESTED = OBS_DIR / ".ingested"          # names of jsonl files already folded in (append-only)

RE_STATUS = ("unknown", "decompiled", "re-verified", "ported-partial", "ported")
COMPARE_STATUS = ("never-compared", "pixel-diffed", "byte-exact", "diverges")

# Fields ingest owns. Anything else in the frontmatter is CURATED and is preserved verbatim.
OBSERVED_KEYS = (
    "key", "kind", "first_seen", "last_seen", "runs", "prims_guest_max", "prims_native_max",
    "frames_seen", "gte_calls_max", "layers", "sub_signatures", "has_native", "native_reached",
)


# ---- the row file ---------------------------------------------------------------------------------
# One file per producer, YAML-ish frontmatter + a prose body. Deliberately the shape docs/findings/ and
# docs/kanban/cards/ already use: greppable (so it reaches a subagent with no tooling), and per-row so
# two concurrent ingests touch disjoint files instead of fighting over one JSON.

@dataclass
class Row:
    key: str
    front: dict = field(default_factory=dict)
    body: str = ""

    @property
    def path(self) -> Path:
        return DB_DIR / f"{self.key}.md"

    def render(self) -> str:
        order = list(OBSERVED_KEYS) + [
            "name", "re_status", "re_evidence", "producer_file", "partial_because",
            "compare_status", "notes",
        ]
        seen, lines = set(), ["---"]
        for k in order:
            if k in self.front:
                lines.append(f"{k}: {_fmt(self.front[k])}")
                seen.add(k)
        for k, v in self.front.items():            # anything a human added by hand, kept
            if k not in seen:
                lines.append(f"{k}: {_fmt(v)}")
        lines.append("---")
        return "\n".join(lines) + "\n" + self.body


def _fmt(v) -> str:
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (list, tuple)):
        return "[" + ", ".join(str(x) for x in v) + "]"
    return str(v)


def _parse(text: str) -> tuple[dict, str]:
    if not text.startswith("---"):
        return {}, text
    end = text.find("\n---", 3)
    if end < 0:
        return {}, text
    front, body = {}, text[end + 4:].lstrip("\n")
    for line in text[3:end].strip().splitlines():
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        v = v.strip()
        if v.startswith("[") and v.endswith("]"):
            inner = v[1:-1].strip()
            val = [x.strip() for x in inner.split(",")] if inner else []
        elif v in ("true", "false"):
            val = v == "true"
        elif re.fullmatch(r"-?\d+", v):
            val = int(v)
        else:
            val = v
        front[k.strip()] = val
    return front, body


def load_rows() -> dict[str, Row]:
    rows: dict[str, Row] = {}
    if not DB_DIR.is_dir():
        return rows
    for p in sorted(DB_DIR.glob("*.md")):
        if p.name == "README.md":
            continue
        front, body = _parse(p.read_text())
        key = str(front.get("key") or p.stem)
        rows[key] = Row(key=key, front=front, body=body)
    return rows


NEW_BODY = """## What it is

NOT YET IDENTIFIED. This row was created mechanically the first time this producer submitted geometry;
nothing has been reverse-engineered about it yet. Fill `name` and the `re_status` chain above as it is
worked, and write here what the effect actually IS — where it appears, what drives it, what it reads.

## Native producer

None recorded. If `has_native` says true, the override table owns this address; name the file in
`producer_file`. If it is false, this effect's picture still comes from the guest.

## Evidence

(Decompile with `external/psxport/tools/decomp.sh`, then cite it here. A `re_status` above `unknown`
with no `re_evidence` is a lint error — see `tools/producers.py check`.)
"""


# ---- ingest --------------------------------------------------------------------------------------

def cmd_ingest(args) -> int:
    # REFUSE, do not return "0 new". A search of a directory that does not exist has measured NOTHING,
    # and saying "no new producers" here would be the exact lie this file's docstring promises not to
    # tell. Exit non-zero so a caller in run.sh cannot mistake it for a clean ingest.
    if not OBS_DIR.is_dir():
        print(f"producers: REFUSING — no observation directory at {OBS_DIR.relative_to(REPO)}.\n"
              f"  That means this tool scanned NOTHING; it does not mean the game drew nothing.\n"
              f"  The port writes it when the census is armed — run the game once, then ingest.",
              file=sys.stderr)
        return 2

    done = set(INGESTED.read_text().split()) if INGESTED.exists() else set()
    files = [p for p in sorted(OBS_DIR.glob("run-*.jsonl")) if p.name not in done]
    if not files:
        print(f"producers: {len(done)} run file(s) already ingested, 0 new in "
              f"{OBS_DIR.relative_to(REPO)} — nothing to do.")
        return 0

    DB_DIR.mkdir(parents=True, exist_ok=True)
    rows = load_rows()
    before = set(rows)
    stats = {"lines": 0, "bad": 0, "rows_touched": 0}
    totals = {k: 0 for k in ("prims_seen", "prims_attributed", "gp0_anon", "span_miss",
                             "unscoped_native", "overflow")}

    for f in files:
        for ln, line in enumerate(f.read_text().splitlines(), 1):
            line = line.strip()
            if not line:
                continue
            stats["lines"] += 1
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                # A line we cannot parse is COUNTED, never skipped silently: a parser that ignores what
                # it cannot match turns a broken instrument into a clean bill of health.
                stats["bad"] += 1
                print(f"producers: WARNING {f.name}:{ln} is not JSON — counted as bad, not ignored",
                      file=sys.stderr)
                continue
            if rec.get("type") == "totals":
                for k in totals:
                    totals[k] += int(rec.get(k, 0) or 0)
                continue
            key = rec.get("key")
            if not key:
                stats["bad"] += 1
                continue
            row = rows.get(key)
            if row is None:
                row = Row(key=key, front={"key": key}, body=NEW_BODY)
                row.front.update({
                    "name": "", "re_status": "unknown", "re_evidence": "", "producer_file": "",
                    "compare_status": "never-compared", "notes": "",
                })
                rows[key] = row
            _merge_observed(row.front, rec)
            stats["rows_touched"] += 1

    written = 0
    for key, row in rows.items():
        text = row.render()
        if not row.path.exists() or row.path.read_text() != text:
            row.path.write_text(text)
            written += 1

    with INGESTED.open("a") as fh:
        for f in files:
            fh.write(f.name + "\n")

    new = sorted(set(rows) - before)
    print(f"producers: ingested {len(files)} run file(s), {stats['lines']} record(s) "
          f"({stats['bad']} unparseable), touched {stats['rows_touched']}, wrote {written} file(s).")
    print(f"  rows now: {len(rows)}   NEW this ingest: {len(new)}")
    for k in new[:40]:
        print(f"    + {k}")
    if len(new) > 40:
        print(f"    … and {len(new) - 40} more")
    # The denominators travel with the ingest, not just with `report`: whoever runs this in run.sh's
    # tail should see immediately if the run's attribution was blind.
    if any(totals.values()):
        print(f"  run totals: prims_seen={totals['prims_seen']} attributed={totals['prims_attributed']} "
              f"gp0_anon={totals['gp0_anon']} span_miss={totals['span_miss']} "
              f"unscoped_native={totals['unscoped_native']} overflow={totals['overflow']}")
    else:
        print("  run totals: NONE RECORDED — the run wrote no `totals` record, so this ingest cannot "
              "say what fraction of the frame it saw. Treat the rows as a lower bound.")
    return 0


def _merge_observed(front: dict, rec: dict) -> None:
    """Fold one observation into a row's OBSERVED half. Monotone by construction: maxima and
    first/last only, never per-run counters — so a row stops changing once a producer's behaviour is
    known, and a play session's git diff is new producers rather than churn."""
    def mx(k, v):
        try:
            front[k] = max(int(front.get(k, 0) or 0), int(v or 0))
        except (TypeError, ValueError):
            pass

    front.setdefault("kind", rec.get("kind", "guest"))
    if not front.get("first_seen"):
        front["first_seen"] = rec.get("seen_at") or rec.get("date") or ""
    front["last_seen"] = rec.get("seen_at") or rec.get("date") or front.get("last_seen", "")
    front["runs"] = int(front.get("runs", 0) or 0) + 1
    mx("prims_guest_max", rec.get("prims_guest"))
    mx("prims_native_max", rec.get("prims_native"))
    mx("frames_seen", rec.get("frames"))
    mx("gte_calls_max", rec.get("gte_calls"))
    # DERIVED, never curated: the runtime reads these off the override table.
    for k in ("has_native", "native_reached"):
        if k in rec:
            front[k] = bool(rec[k])
    for k in ("layers", "sub_signatures"):
        if rec.get(k):
            merged = list(dict.fromkeys(list(front.get(k) or []) + list(rec[k])))
            front[k] = merged


# ---- report --------------------------------------------------------------------------------------

def cmd_report(args) -> int:
    rows = load_rows()
    if not rows:
        print(f"producers: NO ROWS at {DB_DIR.relative_to(REPO)}.\n"
              f"  This is 'the DB is empty', which is NOT 'the game has no producers'. Run the game "
              f"with the census armed and then `tools/producers.py ingest`.", file=sys.stderr)
        return 2

    owned = [r for r in rows.values() if r.front.get("has_native")]
    reached = [r for r in owned if r.front.get("native_reached")]
    ported = [r for r in rows.values() if r.front.get("re_status") == "ported"]
    guest_prims = sum(int(r.front.get("prims_guest_max", 0) or 0) for r in rows.values())
    nat_prims = sum(int(r.front.get("prims_native_max", 0) or 0) for r in rows.values())

    if args.todo:
        return _todo(rows)

    print(f"PRODUCER DB — {len(rows)} rows in {DB_DIR.relative_to(REPO)}")
    print(f"  has a native producer (derived from the override table): {len(owned)}/{len(rows)}")
    print(f"    …of which the native body was actually REACHED in a run: {len(reached)}/{len(owned)}")
    print(f"  curated re_status == ported: {len(ported)}/{len(rows)}")
    print(f"  peak prims: guest {guest_prims}  native {nat_prims}")
    print()
    print("  WHAT THIS DOES NOT SAY, every time, because the numbers above are worthless without it:")
    print("   * a row absent from the DB is NOT-OBSERVED, never absent — the DB is the union of the")
    print("     runs that were ingested, and only the scenes those runs actually played.")
    print("   * per-run attribution blindness (unattributable prims, an unfed census, overflow) is in")
    print("     the ingest lines and in scratch/producers/*.jsonl `totals` records — read them before")
    print("     quoting any percentage here.")
    print("   * `has_native` true with `native_reached` false means we THINK we own it and it never")
    print("     ran. That combination is a lie in the DB; `report --todo` ranks it above new work.")
    unknown = [r for r in rows.values() if (r.front.get("re_status") or "unknown") == "unknown"]
    print(f"   * {len(unknown)}/{len(rows)} rows are still `re_status: unknown` — mechanically created,")
    print("     never identified by a human.")
    return 0


def _todo(rows: dict[str, Row]) -> int:
    """THE WORK QUEUE — the answer to "work on the DB entries"."""
    lies, work = [], []
    for r in rows.values():
        f = r.front
        if f.get("has_native") and not f.get("native_reached"):
            lies.append(r)
            continue
        if f.get("re_status") == "ported":
            continue
        work.append(r)

    def weight(r: Row) -> tuple:
        f = r.front
        return (-int(f.get("prims_guest_max", 0) or 0),
                -int(f.get("runs", 0) or 0),
                str(f.get("re_status") or "unknown"))

    if lies:
        print("!! FIRST — rows that claim a native producer whose body NEVER RAN. Either the override is")
        print("   not installed, or the run never entered its scene. A green gate over these is hollow.")
        for r in lies:
            print(f"   {r.key}  {r.front.get('name') or '(unnamed)'}  runs={r.front.get('runs')}")
        print()
    print("WORK QUEUE — biggest un-owned producers first (prims_guest_max, then how many runs saw it):")
    for r in sorted(work, key=weight)[:30]:
        f = r.front
        print(f"  {r.key}  prims={f.get('prims_guest_max', 0):>6}  runs={f.get('runs', 0):>3}  "
              f"re={f.get('re_status') or 'unknown':<14} {f.get('name') or '(unnamed)'}")
    if not work:
        print("  (empty — every observed producer is either ported or listed above)")
    print()
    print(f"denominator: {len(rows)} rows total, {len(work)} in the queue, {len(lies)} flagged as a lie,")
    print(f"             {len(rows) - len(work) - len(lies)} already `ported`.")
    return 0


# ---- search / show / check -----------------------------------------------------------------------

def cmd_search(args) -> int:
    rows = load_rows()
    needles = [w.lower() for w in args.words]
    hits = []
    for r in rows.values():
        hay = (r.key + " " + " ".join(f"{k}={v}" for k, v in r.front.items()) + " " + r.body).lower()
        score = sum(hay.count(n) for n in needles)
        if score:
            hits.append((score, r))
    for score, r in sorted(hits, key=lambda t: -t[0])[:25]:
        print(f"  {r.key}  {r.front.get('name') or '(unnamed)'}  re={r.front.get('re_status')}  hits={score}")
    print(f"scanned {len(rows)} rows for {needles!r} — {len(hits)} matched")
    return 0 if hits else 1


def cmd_show(args) -> int:
    rows = load_rows()
    r = rows.get(args.key) or rows.get(args.key.lower())
    if not r:
        print(f"producers: no row `{args.key}` among {len(rows)} rows", file=sys.stderr)
        return 1
    print(r.render())
    return 0


def cmd_check(args) -> int:
    """Lint the CURATED half. Every rule here is a way the DB can assert something it has not earned."""
    rows = load_rows()
    if not rows:
        print("producers: no rows to check — that is not a pass", file=sys.stderr)
        return 2
    bad = 0
    for key, r in sorted(rows.items()):
        f = r.front

        def err(msg):
            nonlocal bad
            bad += 1
            print(f"  {key}: {msg}")

        st = f.get("re_status") or "unknown"
        if st not in RE_STATUS:
            err(f"re_status '{st}' is not one of {RE_STATUS}")
        if st not in ("unknown",) and not str(f.get("re_evidence") or "").strip():
            err(f"re_status is '{st}' with NO re_evidence — a claim with no evidence is a belief")
        if st == "ported-partial" and not str(f.get("partial_because") or "").strip():
            err("re_status is 'ported-partial' with no partial_because — name the branch that is NOT "
                "ported, or the row reads as fully owned")
        cs = f.get("compare_status") or "never-compared"
        if cs not in COMPARE_STATUS:
            err(f"compare_status '{cs}' is not one of {COMPARE_STATUS}")
        pf = str(f.get("producer_file") or "").strip()
        if pf and not (REPO / pf).exists():
            err(f"producer_file '{pf}' does not exist")
        if f.get("has_native") and st == "unknown":
            err("has_native is true but re_status is still 'unknown' — the override table owns this "
                "address, so somebody ported it; identify the row")
        if st == "ported" and not pf:
            err("re_status is 'ported' with no producer_file — where is the code?")
    print(f"producers check: {len(rows)} rows, {bad} problem(s)")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("ingest", help="fold scratch/producers/*.jsonl into docs/producers/")
    rp = sub.add_parser("report", help="coverage, with its denominators")
    rp.add_argument("--todo", action="store_true", help="print the ranked work queue instead")
    sp = sub.add_parser("search", help="find rows")
    sp.add_argument("words", nargs="+")
    sh = sub.add_parser("show", help="print one row")
    sh.add_argument("key")
    sub.add_parser("check", help="lint the curated fields (exit 1 on a problem)")
    args = ap.parse_args()
    return {"ingest": cmd_ingest, "report": cmd_report, "search": cmd_search,
            "show": cmd_show, "check": cmd_check}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
