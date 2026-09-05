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
    tools/producers.py stale                  # WHICH CLAIMS THIS BUILD HAS RE-EARNED (kanban #91)
    tools/producers.py stale --selftest       # prove the staleness check fires in BOTH directions

WHAT IS MACHINE-OWNED AND WHAT IS YOURS. A row's frontmatter has two halves and `ingest` only ever
touches the first:

    observed:   first_seen, last_seen, runs, prims_guest_max, prims_native_max, frames_seen,
                gte_calls_max, layers, sub_signatures, has_native, native_reached, owned_query,
                native_hits, oracle_hits
    curated:    name, re_status, re_evidence, producer_file, partial_because, compare_status, notes

`has_native` / `native_reached` are DERIVED from the override table by the runtime, not curated — that
is what stops this from rotting into a wish list. They mean "the GUEST ADDRESS is override-installed",
NOT "a native draws this picture": a display-pass producer legitimately draws from game state while the
guest function stays on the substrate, so `has_native: false` alongside a large `prims_native_max` is
normal and correct. `owned_query: unavailable` is the THIRD state — the run could not ask (no registry
injected) — and is deliberately distinct from a false, because until 2026-08-12 nothing emitted these
fields at all and the report line below could print only 0/N however the run went. Only slow-moving observed fields are tracked at all
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
    "owned_query", "native_hits", "oracle_hits",
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

(Decompile with `external/psxport/the Ghidra evidence workflow`, then cite it here. A `re_status` above `unknown`
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
    # DERIVED, never curated: the runtime reads these off the override table (overrides::query).
    for k in ("has_native", "native_reached"):
        if k in rec:
            front[k] = bool(rec[k])
    # The hit counts behind those booleans, so a reader can tell "never dispatched" (oracle_hits 0)
    # from "dispatched and ran as the substrate reference" (oracle_hits > 0). And `owned_query`, which
    # records that a run could not ASK — kept distinct from a false, because conflating the two is how
    # this whole field group came to be unfalsifiable.
    for k in ("native_hits", "oracle_hits"):
        if k in rec:
            front[k] = int(rec[k])
    if rec.get("owned_query"):
        front["owned_query"] = str(rec["owned_query"])
    elif "has_native" in rec:
        front.pop("owned_query", None)   # a run that COULD ask supersedes an earlier one that could not
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
    # The third state, kept separate from "not installed" on purpose: a row whose runs never had a
    # registry to ask. Counting these as false is what made this metric unfalsifiable in the first place.
    unasked = [r for r in rows.values()
               if r.front.get("owned_query") == "unavailable" and not r.front.get("has_native")]
    drawing = [r for r in rows.values() if int(r.front.get("prims_native_max", 0) or 0) > 0]
    ported = [r for r in rows.values() if r.front.get("re_status") == "ported"]
    guest_prims = sum(int(r.front.get("prims_guest_max", 0) or 0) for r in rows.values())
    nat_prims = sum(int(r.front.get("prims_native_max", 0) or 0) for r in rows.values())

    if args.todo:
        return _todo(rows)

    print(f"PRODUCER DB — {len(rows)} rows in {DB_DIR.relative_to(REPO)}")
    # LABELLED FOR WHAT IT MEASURES. This line used to read "has a native producer", which is a
    # different and much stronger claim than the field supports — and it read as "we own nothing" while
    # every row had a native producer drawing into it.
    print(f"  a NATIVE PRODUCER drew into this row (prims_native > 0): {len(drawing)}/{len(rows)}")
    print(f"  the GUEST ADDRESS is override-installed: {len(owned)}/{len(rows)}"
          f"{f' ({len(unasked)} row(s) never had a registry to ask — not the same as false)' if unasked else ''}")
    print(f"    …of which the installed native was actually REACHED in a run: {len(reached)}/{len(owned)}")
    if not owned and drawing:
        print("    NOTE: 0 override-installed with rows actively drawing is EXPECTED here, not a defect —"
              "\n          these are display-pass producers; the guest functions stay on the substrate.")
    # THE GUEST-ONLY BREAKDOWN. A row with guest prims and NO native prims reads as "this effect has no
    # native producer" — and for a third of them that is FALSE. Reported as three separate states because
    # the undifferentiated count invited exactly the wrong conclusion once already (kanban #89: it was
    # quoted as a ranked work-remaining list, when 11 of 27 rows were natively OWNED emitters). Every
    # number here comes from fields already in the schema; nothing new is measured.
    guest_only = [r for r in rows.values()
                  if int(r.front.get("prims_guest_max", 0) or 0) > 0
                  and int(r.front.get("prims_native_max", 0) or 0) == 0]
    go_owned_unreached = [r for r in guest_only
                          if r.front.get("has_native") and not r.front.get("native_reached")]
    go_owned_reached = [r for r in guest_only
                        if r.front.get("has_native") and r.front.get("native_reached")]
    go_unowned = [r for r in guest_only if not r.front.get("has_native")]
    if guest_only:
        print(f"  GUEST-ONLY rows (the guest drew, no native producer pushed): {len(guest_only)}")
        print(f"    NOT override-installed — no native owns the address: {len(go_unowned)}")
        print(f"    override-installed but the native was NEVER REACHED while the guest drew: "
              f"{len(go_owned_unreached)}")
        print(f"    override-installed AND reached, yet pushed no prims: {len(go_owned_reached)}")
        print("    THIS IS NOT A WORK-REMAINING RANKING, and the middle group is why: a byte-faithful")
        print("    guest-WRITING emitter is natively owned and still produces the picture through the")
        print("    guest packet path, so it can never open a ProducerScope and can never become a claim.")
        print("    An address in the first group may also already have a native reimplementation that")
        print("    installs no override, or a native producer under a DIFFERENT key. Identify a row with")
        print("    `tools/codemap.py --addr <key>` before calling it unported.")
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

    # Rank by the LARGER of the two legs, and say which one it came from. Ranking on prims_guest_max
    # alone was silently useless: the guest leg is a later stage of the feed, so that column is 0 for
    # every row until it lands, and a queue that prints "prims=0" for a producer the run watched emit
    # 51,272 native prims is a work list sorted by nothing. Reporting WHICH leg supplied the number is
    # the point — a big native count with no guest count means "we draw this, but the compare is blind",
    # which is a different job from "the guest draws this and we do not".
    def leg_prims(f: dict) -> tuple:
        g = int(f.get("prims_guest_max", 0) or 0)
        n = int(f.get("prims_native_max", 0) or 0)
        return (g, "guest") if g >= n else (n, "native")

    def weight(r: Row) -> tuple:
        f = r.front
        return (-leg_prims(f)[0],
                -int(f.get("runs", 0) or 0),
                str(f.get("re_status") or "unknown"))

    if lies:
        print("!! FIRST — rows whose guest address IS override-installed but whose native body NEVER RAN.")
        print("   A green gate over these is hollow: nothing exercised the native. `oracle_hits` splits the")
        print("   two causes, which mean different things — 0 means the address was never dispatched at all")
        print("   (its guest path does not execute on the measured leg), >0 means it WAS dispatched and ran")
        print("   as the SUBSTRATE reference instead. Only the first is an unexercised override.")
        for r in lies:
            oh = int(r.front.get("oracle_hits", 0) or 0)
            why = "never dispatched" if oh == 0 else f"ran as substrate reference ({oh} oracle hit(s))"
            print(f"   {r.key}  {r.front.get('name') or '(unnamed)'}  runs={r.front.get('runs')}  — {why}")
        print()
    print("WORK QUEUE — biggest un-owned producers first (max of either leg, then how many runs saw it).")
    print("The `leg` column says WHICH leg the count came from; `native` with no guest count means the")
    print("compare is still blind for that row, not that the guest never drew it.")
    for r in sorted(work, key=weight)[:30]:
        f = r.front
        n, leg = leg_prims(f)
        print(f"  {r.key}  prims={n:>6} ({leg:<6})  runs={f.get('runs', 0):>3}  "
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
        if f.get("owned_query") == "unavailable" and f.get("has_native"):
            err("has_native is set on a row whose ownership query was unavailable — one of the two is "
                "stale; re-ingest a run written by a build that injects overrides::query")
        if f.get("has_native") and st == "unknown":
            err("has_native is true but re_status is still 'unknown' — the override table owns this "
                "address, so somebody ported it; identify the row")
        if st == "ported" and not pf:
            err("re_status is 'ported' with no producer_file — where is the code?")
    print(f"producers check: {len(rows)} rows, {bad} problem(s)")
    return 1 if bad else 0


# ---- stale: PROVENANCE FOR THE CLAIM SET, reconstructed from the run corpus (kanban #91) ----------
#
# THE DEFECT THIS ANSWERS. `scratch/producers/claims.txt` is the set of guest addresses a NATIVE
# producer has keyed prims at, and psxport's ProducerCensus::appendClaims persists it APPEND-ONLY —
# correctly, because a guest leg runs no native producer, so absence on one leg must never un-earn a
# claim. But appendClaims writes the LOADED set unioned with what THIS run earned, so every run
# re-emits a full copy of the file's own contents. Consequence: the file records no provenance at all.
# A claim earned by the present build and a claim left behind by a build whose producer key has since
# MOVED are byte-identical in it, and the newest block always looks freshly earned.
#
# WHY THE PROVENANCE CAN BE RECONSTRUCTED HERE. The per-run JSONL is NOT append-only-unioned: each
# `run-<stamp>.jsonl` holds only what that run actually observed, and a row with `prims_native > 0` is
# precisely an EARN event. So "when was this claim last really earned" is a query over the corpus, and
# the answer can be compared against the code identity of the present build.
#
# WHAT THE NEGATIVE PRINTS, designed before the check (global CLAUDE.md). A claim this build has not
# re-earned is reported as NOT-RE-EARNED and never as dead, because the two are different facts and only
# the first is measured here: a producer's key is a function of GUEST STATE (Tomba! 2's world prims key
# on the per-mode emitter the area's render-mode byte selects), so a claim goes un-earned whenever the
# corpus never visited the content that earns it. Measured instance, and the reason this wording is not
# hedging: 0x800803DC was un-earned by 5 legs and 119 historical native runs, and `warp 9` earned it
# immediately (kanban #91). Every line carries its denominator, and a missing claim file or an empty
# corpus REFUSES with exit 2 rather than reporting a clean "no stale claims".

def _run_git(repo: Path, *args: str) -> str:
    import subprocess
    return subprocess.run(["git", "-C", str(repo), *args],
                          capture_output=True, text=True, check=True).stdout


FRAMEWORK = "external/psxport"
FRAMEWORK_KEY_GREP = r"ProducerScope\|appendClaims\|loadClaims"


def _key_paths(repo: Path) -> tuple[list[str], list[str]]:
    """The game-side paths that DECIDE a producer key: every file that opens a `ProducerScope`."""
    hits = _run_git(repo, "grep", "-l", "ProducerScope", "--", "game", "runtime").split()
    return sorted(set(hits)), hits


def _framework_ref(repo: Path) -> tuple[str, str]:
    """(iso stamp, provenance) for the FRAMEWORK half of the key-deciding code.

    NOT the gitlink, and this is a correction the build gate caught rather than something reasoned out.
    Dating the framework by `git log -- external/psxport` in THIS repo sees only pin bumps, so a framework
    commit that touches nothing but docs fossilises every claim — the very error the reference exists to
    avoid, one level down. Measured instance: pin 7dc380c5 -> 4d218e9f (this repo's 11a75fb, 15:01:02)
    changed 2 docs files and 2 analysis tools, nothing that compiles, and it made the whole corpus refuse.
    So the framework is dated by ITS OWN history over ITS key-deciding files, and the checkout is required
    to match the recorded pin — otherwise the thing being dated is not the thing that was built.
    """
    sub = repo / FRAMEWORK
    if not (sub / ".git").exists():
        # ABSENT is not the same as UNCHECKABLE: a tree with no framework checkout has no framework-side
        # code that could fossilise a claim, so the game-side reference stands alone and says so.
        return "", f"ABSENT — no {FRAMEWORK} checkout in this tree, so nothing framework-side can date a claim"
    pin = _run_git(repo, "ls-tree", "HEAD", FRAMEWORK).split()
    recorded = pin[2] if len(pin) > 2 else ""
    head = _run_git(sub, "rev-parse", "HEAD").strip()
    if recorded and head != recorded:
        return "", (f"UNAVAILABLE — {FRAMEWORK} is checked out at {head[:8]} but the recorded gitlink is "
                    f"{recorded[:8]}; dating against a framework this tree does not record is meaningless")
    files = _run_git(sub, "grep", "-l", FRAMEWORK_KEY_GREP, "--", "runtime").split()
    if not files:
        return "", (f"UNAVAILABLE — no file under {FRAMEWORK}/runtime matches {FRAMEWORK_KEY_GREP}; the "
                    f"census may have moved, and a reference over an empty path set would date nothing")
    out = _run_git(sub, "log", "-1", "--format=%cd|%h", "--date=format:%Y-%m-%dT%H:%M:%S",
                   "--", *files).strip()
    if not out:
        return "", f"UNAVAILABLE — no commit in {FRAMEWORK} touches any of its {len(files)} key path(s)"
    stamp, sha = out.split("|", 1)
    return stamp, (f"framework {FRAMEWORK} @ pin {head[:8]}: newest commit touching its "
                   f"{len(files)} census/scope path(s) is {sha} @ {stamp}")


def _ref_time_from_git(repo: Path = REPO) -> tuple[str, str]:
    """(iso_stamp, provenance) for the code identity a claim must be re-earned under.

    NOT HEAD's commit time: a commit that changes only a findings doc cannot fossilise a claim, so
    HEAD-dating fossilises every claim on every docs commit. The thing that fossilises a claim is a
    change to the code that CHOOSES a producer's key — the files that open a `ProducerScope`, plus the
    framework's own census/scope code (see `_framework_ref`, which is dated by the SUBMODULE's history
    rather than by the gitlink, because a docs-only framework commit must not fossilise anything). So the
    reference is the newest commit over the union of those two sets.

    CORRECTION 2026-08-12, because an earlier version of this docstring recorded an incident that does
    NOT REPRODUCE. It claimed HEAD-dating had been observed calling all 10 claims stale on a corpus in
    which 9 had just been earned. `git reflog` shows HEAD was 11a75fb for the whole measurement window
    (15:23-15:37; committed 15:01:02, superseded 15:53:39) and 11a75fb's only change is
    `external/psxport` — which IS in the key-deciding set. Both references therefore resolve to
    11a75fb @ 2026-08-12T15:01:02 and produce byte-identical output on this tree, so nothing here was
    measured on the real corpus. The DISCRIMINATION is still right, and is now gated hermetically
    instead of by anecdote: `--selftest`'s `docs-only-head` fixture puts a docs commit on top of a
    key-path commit and asserts this function returns the OLDER, key-path one.

    UNCOMMITTED work counts too, and this is not a refinement: the fossil that motivated kanban #91 was
    written by an uncommitted draft. A dirty key-deciding file's mtime therefore raises the reference,
    so a claim earned before that edit cannot read re-earned.
    """
    try:
        files, hits = _key_paths(repo)
        out = _run_git(repo, "log", "-1", "--format=%cd|%h", "--date=format:%Y-%m-%dT%H:%M:%S",
                       "--", *files).strip()
        if not out:
            return "", (f"UNAVAILABLE — no commit touches any of the {len(files)} key-deciding path(s); "
                        f"the grep for ProducerScope matched {len(hits)} file(s)")
        stamp, sha = out.split("|", 1)
        why = (f"newest commit touching the {len(files)} game path(s) that DECIDE a producer key "
               f"(all {len(hits)} open a ProducerScope): {sha} @ {stamp}")
        fw, fw_why = _framework_ref(repo)
        if not fw and not fw_why.startswith("ABSENT"):
            return "", fw_why
        why += f"; {fw_why}"
        if fw > stamp:
            stamp = fw
        dirty = [l[3:] for l in _run_git(repo, "status", "--porcelain", "--", *files).splitlines()
                 if l.strip()]
        if dirty:
            newest, when = "", stamp
            for rel in dirty:
                p = repo / rel.strip().strip('"')
                if not p.exists():
                    continue
                s = _iso(p.stat().st_mtime)
                if s > when:
                    newest, when = rel.strip(), s
            why += (f"; {len(dirty)} of those path(s) are DIRTY in the worktree"
                    + (f", newest edit {newest} @ {when} — the reference is that edit, not the commit, "
                       f"because an uncommitted key change fossilises a claim exactly as a commit does"
                       if newest else " but none is newer than the commit"))
            return when, why
        return stamp, why
    except Exception as e:                                        # noqa: BLE001 — reported, not swallowed
        return "", f"UNAVAILABLE ({type(e).__name__}: {e})"


def _iso(epoch: float) -> str:
    import datetime
    return datetime.datetime.fromtimestamp(epoch).strftime("%Y-%m-%dT%H:%M:%S")


def _build_identity(binary: Path) -> tuple[str, str]:
    """(iso stamp of the binary that ran, provenance) — or ("", why not).

    WHY THIS EXISTS, and it is the check that was missing. `stale` dates a claim by comparing a RUN's
    wall clock against a COMMIT's time, which silently assumes *the binary that produced the run was
    built from that code*. Nothing enforced that, so the one case the whole card exists for read GREEN:
    a run of a STALE or DIRTY binary, started after a key-path commit, was credited as re-earning every
    claim it touched. Dating is only meaningful once the artifact is pinned, so the artifact is now an
    input: the built binary must be NEWER than every key-deciding path, and a run older than the binary
    is not credited.

    RESIDUAL BLIND SPOT, stated because mtime is not a content identity: `touch` fakes it, and a build
    made against a different `PSXPORT_DIR` is invisible here. The real fix is the handover already
    written in kanban #91 — a `PSXPORT_BUILD_ID` from `git describe --always --dirty` compiled into the
    runtime and emitted into every run's JSONL, at which point this function reads the id out of the run
    instead of inferring it from a file date.
    """
    if not binary.exists():
        return "", (f"UNAVAILABLE — no built binary at {binary}. Dating a run against a commit means "
                    f"nothing unless the artifact that produced the run can be pinned to that commit.")
    return _iso(binary.stat().st_mtime), (f"{_rel(binary)} md5 {_md5(binary)[:12]} mtime "
                                          f"{_iso(binary.stat().st_mtime)}")


def _md5(p: Path) -> str:
    import hashlib
    h = hashlib.md5()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _leg_binary(d: Path) -> dict:
    """The binary identity `tools/gate.py` RECORDED for this leg, or {} if the leg predates that.

    An md5 recorded at launch is a real identity; the on-disk binary's mtime is a guess about it, and in
    a checkout several sessions rebuild it is a guess that has already been wrong (see gate.py's note).
    """
    f = d / "binary.txt"
    if not f.is_file():
        return {}
    out = {}
    for line in f.read_text(errors="replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


@dataclass
class Earned:
    """The earn index PLUS every denominator and every thing it could not read."""
    last: dict[str, str] = field(default_factory=dict)            # newest earn by ANY run
    last_credited: dict[str, str] = field(default_factory=dict)   # newest earn by the PRESENT binary
    certified: bool = False
    no_identity: list[str] = field(default_factory=list)
    files: list[tuple[str, int, int, int]] = field(default_factory=list)  # rel, earns, skipped, lines
    legs: int = 0
    credited_legs: int = 0
    skipped_lines: int = 0
    unreadable: list[str] = field(default_factory=list)
    predate_build: list[str] = field(default_factory=list)
    empty_dirs: list[str] = field(default_factory=list)
    collisions: dict[str, list[str]] = field(default_factory=dict)       # same bytes = a lost leg
    collisions_warn: dict[str, list[str]] = field(default_factory=dict)  # same name, different legs

    @property
    def nfiles(self) -> int:
        return len(self.files)


def _earn_index(obs_dirs: list[Path], build_stamp: str = "", build_md5: str = "") -> Earned:
    """addr -> newest run stamp that EARNED it, over one or more LEG directories.

    MULTIPLE DIRECTORIES, not one, and that is a bug fix rather than a feature. The union of 8 legs was
    previously built by `cp`-ing every leg's run file into one directory — and the runtime names them
    `run-<stamp>.jsonl` at ONE-SECOND resolution, so two legs that finished in the same second collided
    and one leg was silently overwritten (measured: warp9 and hutalt both finished 2026-08-12T15:30:49;
    the union held hutalt's file twice and warp9's not at all, and warp9 is the leg that carries the
    proof). Reading the legs where they lie removes the copy, and a basename seen in two directories is
    now REPORTED rather than being a coin flip.

    UNPARSEABLE LINES ARE COUNTED AND PRINTED. A comment used to claim they were "counted below via the
    file/leg denominators"; nothing counted them, so a corpus of half-written JSONL reported a clean,
    fully-denominated result.
    """
    e = Earned()
    seen_names: dict[str, list[str]] = {}
    files: list[Path] = []
    leg_of: dict[Path, Path] = {}
    for d in obs_dirs:
        found = sorted(d.glob("run-*.jsonl"))
        if not found:
            e.empty_dirs.append(str(d))
        for p in found:
            seen_names.setdefault(p.name, []).append(_rel(p))
            files.append(p)
            leg_of[p] = d
    # A basename in two leg dirs is only DATA LOSS when the two are the same bytes — that is the
    # signature of a `cp`-built union, where one leg overwrote another (or the same leg is being counted
    # twice). Two legs that merely finished in the same second write different content, and reading them
    # by full path handles that fine, so it is a WARNING and not a refusal: refusing there would block the
    # very collision-free way of reading the corpus that replaces the copy.
    import hashlib
    for n, v in seen_names.items():
        if len(v) < 2:
            continue
        digests = {hashlib.md5((REPO / p).read_bytes()).hexdigest()[:12] if (REPO / p).exists()
                   else hashlib.md5(Path(p).read_bytes()).hexdigest()[:12] for p in v}
        (e.collisions if len(digests) == 1 else e.collisions_warn)[n] = v
    for p in files:
        stamp = p.name[len("run-"):-len(".jsonl")]
        try:
            text = p.read_text(errors="replace")
        except OSError as ex:
            e.unreadable.append(f"{_rel(p)} ({type(ex).__name__}: {ex})")
            continue
        earned, skipped, nlines = set(), 0, 0
        for line in text.splitlines():
            if not line.strip():
                continue
            nlines += 1
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                skipped += 1
                continue
            if rec.get("kind") != "guest" or not rec.get("prims_native"):
                continue
            earned.add(rec["key"])
        e.skipped_lines += skipped
        e.files.append((_rel(p), len(earned), skipped, nlines))
        if not earned:
            continue
        e.legs += 1
        # CREDITED means "this run is KNOWN to have executed the present binary". Recorded identity
        # decides it when the gate wrote one; otherwise mtime order is the only thing left, and it can
        # only ever DISQUALIFY (a run older than the binary ran something else) — never certify, which is
        # why a leg with no recorded identity lands in the third state rather than in `live`.
        rec = _leg_binary(leg_of[p])
        if rec and "runs" in rec and p.name not in [r for r in rec["runs"].split(",") if r]:
            # THE IDENTITY NAMES WHICH RUNS IT COVERS, and this one is not among them. A leg dir keeps one
            # run file per run but only ONE binary.txt, so without this the newest launch's identity
            # vouches for every older run beside it. Measured: scratch/k91b/warp9 held a 16:22:00 run from
            # an earlier build and the report credited it to the binary built at 16:45:49.
            e.predate_build.append(f"{_rel(p)} is NOT named in {_rel(leg_of[p] / 'binary.txt')}'s "
                                   f"runs= list ({rec['runs'] or 'empty'}) — it was written by an earlier "
                                   f"launch into the same leg dir, so that identity does not cover it")
        elif rec and "runs" not in rec and len([q for q in files if leg_of[q] == leg_of[p]]) > 1:
            # A legacy record (no runs= line) in a dir holding several runs cannot say which run it is
            # about. Crediting any of them would be a coin flip, so none is credited.
            e.predate_build.append(f"{_rel(p)} shares a leg dir with other run file(s) and "
                                   f"{_rel(leg_of[p] / 'binary.txt')} predates the runs= field, so it "
                                   f"cannot say WHICH run used that binary — re-run the leg into an "
                                   f"empty dir through tools/gate.py")
        elif rec:
            if rec.get("status") != "OK":
                e.predate_build.append(f"{_rel(p)} ran a binary that was REPLACED MID-RUN "
                                       f"({rec.get('md5','?')[:12]} -> {rec.get('md5_after','?')[:12]})")
            elif build_md5 and rec.get("md5") == build_md5:
                e.credited_legs += 1
                e.certified = True
                for k in earned:
                    if stamp > e.last_credited.get(k, ""):
                        e.last_credited[k] = stamp
            else:
                e.predate_build.append(f"{_rel(p)} ran binary md5 {rec.get('md5','?')[:12]}, the present "
                                       f"binary is {build_md5[:12]} — a DIFFERENT build")
        elif build_stamp and stamp < build_stamp:
            e.predate_build.append(f"{_rel(p)} (ran {stamp}, binary on disk built {build_stamp}, and this "
                                   f"leg recorded no binary identity)")
        else:
            e.no_identity.append(f"{_rel(p)} (ran {stamp}, no binary.txt — run it through tools/gate.py "
                                 f"so the leg records which binary produced it)")
        for k in earned:
            if stamp > e.last.get(k, ""):
                e.last[k] = stamp
    return e


def _rel(p: Path) -> str:
    try:
        return str(p.resolve().relative_to(REPO))
    except ValueError:
        return str(p)


def _stale_report(claims_path: Path, obs_dirs: list[Path], ref: str, ref_why: str,
                  build: str = "", build_why: str = "", build_md5: str = "") -> tuple[int, dict]:
    if not claims_path.is_file():
        print(f"producers stale: REFUSING — no claim set at {claims_path}.\n"
              f"  NOTHING was checked. This is not 'no stale claims'; the file a guest leg resolves\n"
              f"  against does not exist, so every guest prim would read as 'no native producer'.",
              file=sys.stderr)
        return 2, {}
    missing = [str(d) for d in obs_dirs if not d.is_dir()]
    if missing or not obs_dirs:
        print(f"producers stale: REFUSING — no run corpus at {', '.join(missing) or '(no --obs given)'}.\n"
              f"  A claim's provenance is reconstructed FROM the per-run JSONL; with no corpus this\n"
              f"  tool can say nothing about any claim, stale or live.", file=sys.stderr)
        return 2, {}
    if not build:
        print(f"producers stale: REFUSING — no build identity ({build_why}).\n"
              f"  This tool dates a RUN against a COMMIT, which only means something if the binary that\n"
              f"  produced the run was built from that commit. With no artifact to pin, every claim\n"
              f"  would read re-earned on the strength of a wall clock alone. NOTHING was checked.",
              file=sys.stderr)
        return 2, {}
    if ref and build < ref:
        print(f"producers stale: REFUSING — the built binary PREDATES the code that decides a producer "
              f"key.\n  binary:    {build_why}\n  reference: {ref_why}\n"
              f"  No run of that binary can have re-earned anything under the present tree, so a green\n"
              f"  report here would be the exact false negative this check exists to prevent. Rebuild\n"
              f"  (`cmake --build build --target tomba2_port`) and re-run the legs.", file=sys.stderr)
        return 2, {}
    # The claim file is append-only AND re-emits its own loaded contents every run, so it holds one
    # block per run. Dedupe to the SET, and print both numbers: the line count is the run count, and
    # conflating them is how a 10-address set reads as 106 claims.
    raw = [l.strip() for l in claims_path.read_text(errors="replace").splitlines() if l.strip()]
    claims: list[str] = []
    for line in raw:
        if line[:2].upper() == "0X":
            a = "0x" + line[2:].upper()
            if a not in claims:
                claims.append(a)
    if not claims:
        print(f"producers stale: REFUSING — {claims_path} holds 0 parseable `0x…` claim line(s).\n"
              f"  An empty claim set is not a clean bill of health; it is a DB with nothing to join to.",
              file=sys.stderr)
        return 2, {}
    e = _earn_index(obs_dirs, build, build_md5)
    last = e.last
    if e.collisions:
        print(f"producers stale: REFUSING — {len(e.collisions)} run-file basename(s) appear in more than "
              f"one\n  leg directory with IDENTICAL BYTES, which means one leg was overwritten by a copy "
              f"(or is\n  being counted twice):", file=sys.stderr)
        for n, v in sorted(e.collisions.items()):
            print(f"    {n}: {', '.join(v)}", file=sys.stderr)
        print("  The runtime stamps run files at ONE-SECOND resolution, which is how a `cp`-built union\n"
              "  silently lost a leg (kanban #91: warp9 and hutalt both finished 15:30:49; the union held\n"
              "  hutalt's file twice and warp9's not at all). Pass each leg as its own --obs and leave the\n"
              "  files where they were written.", file=sys.stderr)
        return 2, {}
    if e.legs == 0:
        print(f"producers stale: REFUSING — scanned {e.nfiles} run file(s) in "
              f"{', '.join(_rel(d) for d in obs_dirs)} and found 0 with a\n"
              f"  native earn (every row prims_native == 0, or every run predates the binary). That is a\n"
              f"  corpus that cannot date ANY claim. Run a pc_render leg first.", file=sys.stderr)
        return 2, {}
    if not ref:
        print(f"producers stale: REFUSING — no reference build time ({ref_why}). Without one there is\n"
              f"  nothing to call a claim stale RELATIVE TO. Pass --ref-time <ISO stamp>.", file=sys.stderr)
        return 2, {}

    # THE CUTOFF IS THE BINARY, not the commit, and that is the fix to this tool's core unstated
    # assumption. Dating a run's wall clock against a COMMIT assumed "the run happened after the commit,
    # therefore the binary was built from it" — so a run of a stale or dirty binary read RE-EARNED and a
    # genuine fossil read green. The chain is now checked end to end instead: the code identity must be
    # no newer than the BINARY (refused above), and a claim is re-earned only by a run of THAT binary.
    # THREE states on the provenance axis, not two, because mtime cannot identify the binary a PAST run
    # used. A claim last earned before the reference is a fossil; one earned by a run of the PRESENT
    # binary is re-earned; one earned in between ran SOME EARLIER BUILD whose identity is not recoverable
    # from these artifacts — calling that re-earned is the false negative this check exists to stop, and
    # calling it a fossil is a different lie. It gets its own state and still exits non-zero.
    live = [c for c in claims if e.last_credited.get(c, "") >= ref]
    unknown = [c for c in claims if c not in live and c in last and last[c] >= ref]
    old = [c for c in claims if c in last and last[c] < ref]
    never = [c for c in claims if c not in last]
    print(f"producers stale: {len(claims)} distinct claim(s) over {len(raw)} line(s) in "
          f"{_rel(claims_path)} (append-only: one block per run), dated against "
          f"{e.nfiles} run file(s) / {e.legs} native leg(s), {e.credited_legs} of them from the "
          f"present binary, over {len(obs_dirs)} leg dir(s)")
    print(f"  reference code identity: {ref_why}")
    print(f"  build identity of the runs: {build_why}")
    print(f"  a claim counts as RE-EARNED only if a leg whose RECORDED binary md5 equals the present "
          f"one earned it,\n    and only at or after the reference. A run's own recorded identity, not a "
          f"file date: dating a run\n    against a commit assumed the run's binary was built from it, and "
          f"nothing checked that.")
    # EVERY LEG, NAMED. The 7-leg table this tool first produced was not the whole corpus: an eighth leg
    # was undisclosed and load-bearing in the union, and a ninth had CRASHED and was omitted entirely.
    # An omitted leg is a dropped negative, so the corpus is now enumerated rather than summarised.
    print(f"  the corpus, leg by leg ({e.nfiles} run file(s) — every one, including the ones that earn "
          f"nothing):")
    for rel, earns, skipped, nlines in e.files:
        tail = f"  [{skipped} UNPARSEABLE line(s)]" if skipped else ""
        print(f"    {rel}  {nlines} row(s), earned {earns}{tail}")
    print(f"  unparseable JSONL lines skipped: {e.skipped_lines} of "
          f"{sum(n for _, _, _, n in e.files)} row(s) scanned"
          + ("  <-- a half-written corpus; the earn index is a LOWER BOUND" if e.skipped_lines else ""))
    if e.collisions_warn:
        print(f"  WARNING: {len(e.collisions_warn)} run-file basename(s) appear in two leg dirs with "
              f"DIFFERENT bytes —\n    two distinct legs finished in the same second. Read by full path "
              f"here, so nothing is lost, but\n    copying these into one directory WOULD lose one:")
        for n, v in sorted(e.collisions_warn.items()):
            print(f"    {n}: {', '.join(v)}")
    if e.unreadable:
        print(f"  run file(s) that could NOT be read: {len(e.unreadable)}")
        for u in e.unreadable:
            print(f"    {u}")
    if e.predate_build:
        print(f"  run file(s) NOT credited — they did not run the present binary: {len(e.predate_build)}")
        for u in e.predate_build:
            print(f"    {u}")
    if e.no_identity:
        print(f"  run file(s) with NO recorded binary identity: {len(e.no_identity)}  <-- these cannot "
              f"certify anything")
        for u in e.no_identity:
            print(f"    {u}")
    if e.empty_dirs:
        print(f"  leg dir(s) holding NO run file at all: {len(e.empty_dirs)}  <-- A LEG THAT WROTE "
              f"NOTHING IS A DROPPED NEGATIVE,")
        for d in e.empty_dirs:
            print(f"    {d}  (the leg crashed, was never run, or the census never armed — find out which)")
    print(f"  RE-EARNED by a run at or after the reference: {len(live)}")
    for c in sorted(live):
        print(f"    {c}  last earned {last[c]}")
    print(f"  BUILD PROVENANCE UNKNOWN — earned after the key-deciding code, but not by a run provably of "
          f"the present binary: {len(unknown)}")
    for c in sorted(unknown):
        print(f"    {c}  last earned {last[c]}  <-- re-run that leg through tools/gate.py to settle it")
    if unknown:
        print(f"    These are NOT certified and NOT fossils, and conflating either way is the defect this "
              f"state exists for.\n    Settle one by re-running its leg through tools/gate.py against the "
              f"present binary (the gate records the\n    binary's md5 into the leg dir). The state "
              f"disappears entirely once the PSXPORT_BUILD_ID handover in\n    kanban #91 lands and every "
              f"run carries its own build identity.")
    print(f"  NOT re-earned since the reference: {len(old)}")
    for c in sorted(old):
        print(f"    {c}  last earned {last[c]}  <-- may be a FOSSIL of a moved producer key")
    print(f"  NEVER earned anywhere in this corpus: {len(never)}")
    for c in sorted(never):
        print(f"    {c}  <-- no run file in this corpus earned it; it was loaded from an outside DB")
    print("  BLIND SPOT, stated because it is the whole difficulty: NOT-RE-EARNED is not DEAD. A "
          "producer's key\n"
          "  is a function of guest state (world prims key on the per-mode emitter the area's render-mode "
          "byte\n"
          "  selects), so a claim goes un-earned whenever this corpus never visited the content that "
          "earns it.\n"
          "  Measured: 0x800803DC read NOT-RE-EARNED over 119 legs and was earned on the first `warp 9`\n"
          "  (kanban #91). Confirm by REACHING the content before you call any claim dead, and never "
          "prune\n"
          "  on this report alone.")
    return (1 if (old or never or unknown) else 0), {"live": live, "old": old, "never": never,
                                          "unknown": unknown, "ref": ref, "build": build,
                                          "skipped": e.skipped_lines, "legs": e.legs,
                                          "credited_legs": e.credited_legs,
                                          "empty_dirs": e.empty_dirs, "files": e.files,
                                          "predate_build": e.predate_build}


PORT_BINARY = REPO / "scratch" / "bin" / "tomba2_port"


def run_stale(claims_path: Path, obs_dirs: list[Path], binary: Path, repo: Path = REPO,
              ref_override: str = "") -> tuple[int, dict]:
    """THE SHIPPING PATH, and the only one. `--selftest` drives THIS, over a fixture git repo and a
    fixture binary, so the git-derived reference and the build-identity gate are covered by the gate
    instead of being injected past it. The previous selftest passed an explicit reference string, which
    left `_ref_time_from_git` — the novel, error-prone half — with zero coverage: a verifier replaced its
    body with a 1970 constant and the real report FLIPPED from "10 NOT re-earned, exit 1" to "10
    RE-EARNED, exit 0" while `--selftest` still said 0 FAILs."""
    ref, why = ((ref_override, f"--ref-time {ref_override}") if ref_override
                else _ref_time_from_git(repo))
    build, build_why = _build_identity(binary)
    return _stale_report(claims_path, obs_dirs, ref, why, build, build_why,
                         _md5(binary) if binary.exists() else "")


def _selftest_corpus(root: Path, claim: str, earn_stamps: list[str], garbage: bool = False,
                     record: Path | None = None, status: str = "OK",
                     cover: list[str] | None = None, legacy: bool = False) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    for p in root.glob("*"):
        if p.is_file():
            p.unlink()
    (root / "claims.txt").write_text(claim + "\n0xDEADBEE0\n")
    for s in earn_stamps:
        body = (json.dumps({"key": claim, "kind": "guest", "prims_guest": 0, "prims_native": 7,
                            "seen_at": s}) + "\n"
                + json.dumps({"key": "0xDEADBEE0", "kind": "guest", "prims_guest": 5,
                              "prims_native": 0, "seen_at": s}) + "\n")
        if garbage:
            body += '{"key":"0xBADF00D","kind":"guest","prims_nativ\n'   # a half-written line
        (root / f"run-{s}.jsonl").write_text(body)
    if record is not None:
        # Exactly what tools/gate.py writes into a leg dir, so the consumer side is gated on the real
        # format rather than on a paraphrase of it.
        covered = earn_stamps if cover is None else cover
        (root / "binary.txt").write_text(
            f"status={status}\nmd5={_md5(record)}\nmtime={_iso(record.stat().st_mtime)}\n"
            f"path={_rel(record)}\nmd5_after={_md5(record) if status == 'OK' else '0' * 32}\n"
            # `legacy` reproduces a record written BEFORE the runs= field existed, which is the only
            # reason the no-runs= branch in `_earn_index` exists.
            + ("" if legacy else f"runs={','.join(f'run-{s}.jsonl' for s in covered)}\n"))
    return root


def _selftest_repo(root: Path, key_time: str, docs_time: str = "", desync_pin: bool = False,
                   fw_time: str = "2025-12-01T00:00:00") -> Path:
    """A throwaway git repo with the same key-deciding SHAPE as this one: a file that opens a
    ProducerScope, plus `.gitmodules` + `external/psxport`. Committed at CONTROLLED dates so the
    reference this tool derives is a known value rather than whatever HEAD happens to be — which is
    exactly what the real tree could not provide, because its HEAD at measurement time was a psxport pin
    bump, i.e. itself a key-deciding path."""
    import shutil
    import subprocess
    if root.exists():
        shutil.rmtree(root)
    (root / "game" / "render").mkdir(parents=True)
    (root / "docs").mkdir(parents=True)
    fw = root / FRAMEWORK
    (fw / "runtime" / "guest instruction path").mkdir(parents=True)
    (fw / "docs").mkdir(parents=True)
    (root / "game" / "render" / "render_walk.cpp").write_text("ProducerScope scope(key);\n")
    (root / ".gitmodules").write_text('[submodule "external/psxport"]\n')
    (fw / "runtime" / "guest instruction path" / "producer_db.cpp").write_text("void appendClaims() {}\n")
    env = dict(os.environ, GIT_AUTHOR_NAME="selftest", GIT_AUTHOR_EMAIL="selftest@local",
               GIT_COMMITTER_NAME="selftest", GIT_COMMITTER_EMAIL="selftest@local")

    def commit(msg: str, when: str, where: Path = root) -> None:
        subprocess.run(["git", "-C", str(where), "add", "-A"], check=True, capture_output=True)
        subprocess.run(["git", "-C", str(where), "commit", "-q", "-m", msg],
                       check=True, capture_output=True,
                       env=dict(env, GIT_AUTHOR_DATE=when, GIT_COMMITTER_DATE=when))

    # A REAL nested repo, so the framework half of the reference — and the pin-vs-checkout integrity
    # check — are exercised rather than assumed. `fw_time` defaults OLDER than the game's key commit, so
    # the union reference is the game's and each class below tests one thing at a time; the
    # framework-newest class passes a later `fw_time` to gate the union itself (see class 12).
    subprocess.run(["git", "-C", str(fw), "init", "-q", "-b", "main"], check=True, capture_output=True)
    commit("framework census", fw_time, fw)
    subprocess.run(["git", "-C", str(root), "init", "-q", "-b", "main"], check=True, capture_output=True)
    commit("key path", key_time)
    if desync_pin:
        # The framework checkout moves AFTER the parent recorded its gitlink: the tree is now dating a
        # framework it does not record, which is the workspace's own known failure mode.
        (fw / "docs" / "moved.md").write_text("checked out past the recorded pin\n")
        commit("framework moved past the pin", "2026-02-01T00:00:00", fw)
    if docs_time:
        (root / "docs" / "findings.md").write_text("a docs-only change cannot fossilise a claim\n")
        commit("docs only", docs_time)
    return root


def _selftest_binary(path: Path, when: str) -> Path:
    import datetime
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"not a real binary; stands in for one. unique per fixture: {path.name} {when}\n")
    t = datetime.datetime.strptime(when, "%Y-%m-%dT%H:%M:%S").timestamp()
    os.utime(path, (t, t))
    return path


def _selftest_stale() -> int:
    """Gate the SHIPPING path — `run_stale`, git-derived reference and build gate included — in both
    directions, over fixture repos whose commit dates are known. Every class below is chosen so that
    breaking one specific mechanism turns it RED; the mechanisms are the git reference, the docs-vs-key
    discrimination, the build-identity gate, the dirty-worktree reference, the unparseable-line count,
    the empty-leg disclosure, and the refusals."""
    base = REPO / "scratch" / "producers-selftest"
    base.mkdir(parents=True, exist_ok=True)
    KEY = "2026-01-02T00:00:00"          # the key-deciding commit
    DOCS = "2026-01-05T00:00:00"         # a later DOCS-ONLY commit, which must NOT move the reference
    fails: list[str] = []

    def check(cond: bool, msg: str) -> None:
        if not cond:
            fails.append(msg)

    # 1+2: the two claim classes, over a repo whose newest commit IS the key path. The positive earns
    # after it, the negative only before. A reference stuck at a constant (e.g. epoch) makes the
    # negative read live and fails here — that is the sabotage the old selftest could not see.
    for label, stamps, want_live, want_old in (
            ("positive/re-earned", ["2026-01-01T00:00:00", "2026-01-03T00:00:00"], True, False),
            ("negative/fossil",    ["2026-01-01T00:00:00"],                        False, True)):
        name = label.split("/")[0]
        repo = _selftest_repo(base / f"repo-{name}", KEY)
        binary = _selftest_binary(base / f"bin-{name}", "2026-01-02T00:00:01")
        d = _selftest_corpus(base / name, "0x800803DC", stamps, garbage=True, record=binary)
        rc, res = run_stale(d / "claims.txt", [d], binary, repo)
        if not res:
            fails.append(f"{label}: the report REFUSED (rc={rc}) on a corpus that exists")
            continue
        check(res["ref"] == KEY, f"{label}: reference {res['ref']!r}, want the key-path commit {KEY!r}")
        got_live, got_old = "0x800803DC" in res["live"], "0x800803DC" in res["old"]
        check(got_live == want_live and got_old == want_old,
              f"{label}: live={got_live} (want {want_live}) stale={got_old} (want {want_old})")
        check("0xDEADBEE0" in res["never"],
              f"{label}: a guest-only row (prims_native 0) was NOT reported as never-earned — the tool "
              f"is counting rows, not earns")
        check(res["skipped"] == len(stamps),
              f"{label}: {res['skipped']} unparseable line(s) counted, want {len(stamps)} — a "
              f"half-written corpus must not report a clean result")
        if want_old:
            check(rc == 1, f"{label}: a stale claim exited {rc} — the report cannot fail")

    # 3: THE DISCRIMINATION `_ref_time_from_git` EXISTS FOR, which nothing gated before. HEAD is a
    # docs-only commit; the reference must be the OLDER key-path commit, so a run between the two reads
    # RE-EARNED. HEAD-dating would call it a fossil.
    repo = _selftest_repo(base / "repo-docshead", KEY, DOCS)
    binary = _selftest_binary(base / "bin-docshead", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "docshead", "0x800803DC", ["2026-01-03T00:00:00"], record=binary)
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and res.get("ref") == KEY,
          f"docs-only-head: reference {res.get('ref')!r}, want the key-path commit {KEY!r} — a "
          f"docs-only HEAD must not fossilise a claim")
    check(bool(res) and "0x800803DC" in res.get("live", []),
          f"docs-only-head: rc={rc}, live={res.get('live')} — want the claim RE-EARNED")

    # 4: a DIRTY key-deciding file raises the reference. The fossil that motivated kanban #91 was
    # written by an uncommitted draft, so an uncommitted key edit that is NEWER than the binary must
    # refuse: the shipped binary does not contain the code that now decides the key.
    repo = _selftest_repo(base / "repo-dirty", KEY)
    edited = repo / "game" / "render" / "render_walk.cpp"
    edited.write_text("ProducerScope scope(other_key);\n")
    binary = _selftest_binary(base / "bin-dirty", "2026-01-02T00:00:01")
    os.utime(edited, (binary.stat().st_mtime + 3600,) * 2)
    d = _selftest_corpus(base / "dirty", "0x800803DC", ["2026-01-09T00:00:00"])
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(rc == 2 and not res,
          f"dirty-key-path: rc={rc} (want 2) — an uncommitted key-deciding edit newer than the binary "
          f"means the run did not exercise the code that now chooses the key")

    # 5: THE BUILD-IDENTITY GATE, the check that was missing entirely. A binary older than the
    # key-deciding code must REFUSE, not report green off the run's wall clock.
    repo = _selftest_repo(base / "repo-oldbin", KEY)
    d = _selftest_corpus(base / "oldbin", "0x800803DC", ["2026-01-03T00:00:00"])
    binary = _selftest_binary(base / "bin-oldbin", "2026-01-01T00:00:00")
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(rc == 2 and not res,
          f"stale-binary: rc={rc} (want 2) — a run of a binary built BEFORE the key-deciding code must "
          f"not be credited")

    # 6: a LEGACY leg that recorded no binary identity certifies NOTHING, whichever side of the binary's
    # own date it ran on. mtime order can only disqualify (the older run), never certify (the newer one).
    repo = _selftest_repo(base / "repo-oldrun", KEY)
    d = _selftest_corpus(base / "oldrun", "0x800803DC", ["2026-01-03T00:00:00", "2026-01-09T00:00:00"])
    binary = _selftest_binary(base / "bin-oldrun", "2026-01-05T00:00:00")
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and len(res.get("predate_build", [])) == 1 and res.get("credited_legs") == 0
          and res.get("legs") == 2 and "0x800803DC" in res.get("unknown", []),
          f"legacy leg, no recorded identity: predate_build={res.get('predate_build')} "
          f"credited={res.get('credited_legs')} legs={res.get('legs')} unknown={res.get('unknown')} — "
          f"want 2 earning legs, 0 credited, the claim UNKNOWN")

    # 6c: the gate recorded that the binary was REPLACED MID-RUN (a sibling session rebuilt the shared
    # tree — measured in this very session). That leg's observations belong to no identifiable build.
    repo = _selftest_repo(base / "repo-swap", KEY)
    binary = _selftest_binary(base / "bin-swap", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "swap", "0x800803DC", ["2026-01-03T00:00:00"], record=binary,
                         status="MISMATCH")
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and res.get("credited_legs") == 0 and "0x800803DC" in res.get("unknown", []),
          f"binary swapped mid-run: credited={res.get('credited_legs')} unknown={res.get('unknown')} — a "
          f"leg whose binary changed under it must certify nothing")

    # 6b: THE THIRD STATE. Earned after the key-deciding commit but before the present binary was built:
    # the run's build cannot be identified from these artifacts, so it must read UNKNOWN — neither
    # re-earned (the old lie) nor fossil (a new one) — and must still exit non-zero.
    repo = _selftest_repo(base / "repo-unknownbuild", KEY)
    d = _selftest_corpus(base / "unknownbuild", "0x800803DC", ["2026-01-03T00:00:00"])
    binary = _selftest_binary(base / "bin-unknownbuild", "2026-01-05T00:00:00")
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and "0x800803DC" in res.get("unknown", []) and rc == 1,
          f"unknown build provenance: rc={rc} unknown={res.get('unknown')} live={res.get('live')} — a run "
          f"that predates the present binary must be neither certified nor called a fossil")

    # 7: a leg directory that wrote NOTHING (the crashed leg) is disclosed, never silently dropped.
    repo = _selftest_repo(base / "repo-crashleg", KEY)
    binary = _selftest_binary(base / "bin-crashleg", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "crashleg", "0x800803DC", ["2026-01-03T00:00:00"], record=binary)
    crashed = base / "crashleg-empty"
    crashed.mkdir(parents=True, exist_ok=True)
    for p in crashed.glob("*"):
        p.unlink()
    rc, res = run_stale(d / "claims.txt", [d, crashed], binary, repo)
    check(bool(res) and len(res.get("empty_dirs", [])) == 1,
          f"crashed leg: empty_dirs={res.get('empty_dirs')} — a leg that wrote nothing is a dropped "
          f"negative and must be named")

    # 6d: A LEG DIR HOLDING TWO RUNS. `binary.txt` is one file per dir but run files accumulate, so an
    # identity written by the newest launch must vouch ONLY for the run(s) that launch produced. Measured
    # on the real tree: scratch/k91b/warp9 held a 16:22:00 run made from an earlier build, and the report
    # credited it to the binary built at 16:45:49 — a build that did not exist when it ran.
    repo = _selftest_repo(base / "repo-twoinadir", KEY)
    binary = _selftest_binary(base / "bin-twoinadir", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "twoinadir", "0x800803DC",
                         ["2026-01-03T00:00:00", "2026-01-09T00:00:00"], record=binary,
                         cover=["2026-01-09T00:00:00"])
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and res.get("credited_legs") == 1 and res.get("legs") == 2
          and len(res.get("predate_build", [])) == 1 and "0x800803DC" in res.get("live", []),
          f"two runs, one identity: credited={res.get('credited_legs')} legs={res.get('legs')} "
          f"predate={res.get('predate_build')} — want exactly the run named in runs= credited")

    # 6e: the same shape with a LEGACY record (written before runs= existed) certifies NOTHING, because
    # nothing in it says which of the two runs used that binary.
    d = _selftest_corpus(base / "legacyrec", "0x800803DC",
                         ["2026-01-03T00:00:00", "2026-01-09T00:00:00"], record=binary, legacy=True)
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and res.get("credited_legs") == 0 and "0x800803DC" in res.get("unknown", []),
          f"legacy identity over two runs: credited={res.get('credited_legs')} "
          f"unknown={res.get('unknown')} — a record that cannot name its run must certify nothing")

    # 8: a basename in two leg dirs REFUSES — the collision that lost warp9 out of the union.
    a = _selftest_corpus(base / "collide-a", "0x800803DC", ["2026-01-03T00:00:00"])
    b = _selftest_corpus(base / "collide-b", "0x800803DC", ["2026-01-03T00:00:00"])
    rc, res = run_stale(a / "claims.txt", [a, b], binary=_selftest_binary(
        base / "bin-collide", "2026-01-02T00:00:01"), repo=_selftest_repo(base / "repo-collide", KEY))
    check(rc == 2 and not res,
          f"basename collision: rc={rc} (want 2) — two legs sharing a run-file name must refuse, not "
          f"let one shadow the other")

    # 9: the FRAMEWORK checkout has moved past the recorded pin, so the code being dated is not the code
    # the tree records. Refuse — `sync-submodules.sh` is a known-defective certifier of exactly this.
    repo = _selftest_repo(base / "repo-desync", KEY, desync_pin=True)
    binary = _selftest_binary(base / "bin-desync", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "desync", "0x800803DC", ["2026-01-03T00:00:00"], record=binary)
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(rc == 2 and not res,
          f"pin desync: rc={rc} (want 2) — a framework checkout ahead of the recorded gitlink must not be "
          f"dated as if it were the recorded one")

    # 12: THE FRAMEWORK HALF OF THE UNION REFERENCE, which nothing gated — every other class puts the
    # framework commit OLDER than the game's, so `if fw > stamp: stamp = fw` in `_ref_time_from_git`
    # could be deleted outright and this selftest stayed green at 0 FAILs (found by sabotage, 2026-08-12;
    # on the real tree that deletion moved the effective reference 9 hours earlier). A framework
    # census/scope change MUST fossilise a claim earned before it, exactly as a game-side one does.
    # Asserted as a BEHAVIOURAL flip, not just a reference string: the leg ran after the game's key
    # commit but before the framework's, so it reads re-earned iff the framework half is ignored.
    FW_NEW = "2026-01-06T00:00:00"
    repo = _selftest_repo(base / "repo-fwnew", KEY, fw_time=FW_NEW)
    binary = _selftest_binary(base / "bin-fwnew", "2026-01-07T00:00:00")
    d = _selftest_corpus(base / "fwnew", "0x800803DC", ["2026-01-04T00:00:00"], record=binary)
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    check(bool(res) and res.get("ref") == FW_NEW,
          f"framework-newest: reference {res.get('ref')!r}, want the FRAMEWORK commit {FW_NEW!r} — the "
          f"reference is the union of the game and framework key paths, not the game's alone")
    check(bool(res) and "0x800803DC" in res.get("old", []) and "0x800803DC" not in res.get("live", []),
          f"framework-newest: rc={rc}, old={res.get('old')} live={res.get('live')} — a leg that ran BEFORE the "
          f"framework's census change must read as a fossil; reading it live means the framework half of "
          f"the reference is being ignored")

    # 13: THE BOUNDARY the printed contract states — "at or after the reference". A leg stamped at
    # EXACTLY the reference second is RE-EARNED; `>=` narrowed to `>` flips it to a fossil with no class
    # objecting (also found by sabotage). One second wide, but it is the stated semantics.
    repo = _selftest_repo(base / "repo-boundary", KEY)
    binary = _selftest_binary(base / "bin-boundary", "2026-01-02T00:00:01")
    d = _selftest_corpus(base / "boundary", "0x800803DC", [KEY], record=binary)
    rc, res = run_stale(d / "claims.txt", [d], binary, repo)
    # (rc is not asserted here: every fixture corpus carries a deliberately never-earned guest row, so
    # the exit status is 1 for reasons unrelated to this boundary. Membership is the discriminator.)
    check(bool(res) and "0x800803DC" in res.get("live", []) and "0x800803DC" not in res.get("old", []),
          f"reference-boundary: rc={rc}, live={res.get('live')} old={res.get('old')} — a leg "
          f"stamped at exactly the reference ({KEY}) is 'at or after' it and must read RE-EARNED")

    # 10+11: the refusals. A missing corpus, and a missing binary.
    repo = _selftest_repo(base / "repo-refuse", KEY)
    rc, _ = run_stale(base / "nope" / "claims.txt", [base / "nope"],
                      _selftest_binary(base / "bin-refuse", "2026-01-02T00:00:01"), repo)
    check(rc == 2, f"missing-corpus: exit {rc}, expected 2 (must refuse, not return empty)")
    d = _selftest_corpus(base / "nobin", "0x800803DC", ["2026-01-03T00:00:00"])
    rc, _ = run_stale(d / "claims.txt", [d], base / "bin-does-not-exist", repo)
    check(rc == 2, f"missing-binary: exit {rc}, expected 2 — with no artifact to pin, a wall clock alone "
                   f"must not certify anything")

    print(f"producers stale --selftest: 17 class(es) gated over the SHIPPING path (run_stale, git "
          f"reference and build gate included), {len(fails)} FAIL(s)")
    for f in fails:
        print(f"  FAIL {f}")
    return 1 if fails else 0


def cmd_stale(args) -> int:
    if args.selftest:
        return _selftest_stale()
    obs = [Path(o) for o in args.obs] if args.obs else [OBS_DIR]
    rc, _ = run_stale(Path(args.claims) if args.claims else OBS_DIR / "claims.txt", obs,
                      Path(args.binary) if args.binary else PORT_BINARY,
                      REPO, args.ref_time)
    return rc


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
    st = sub.add_parser("stale", help="which claims THIS build has re-earned (exit 1 if any has not)")
    st.add_argument("--claims", default="", help="claim set to date (default scratch/producers/claims.txt)")
    st.add_argument("--obs", action="append", default=[],
                    help="a LEG directory of run-*.jsonl (repeatable; default scratch/producers/). "
                         "Pass each leg separately — do NOT cp several legs into one directory, the "
                         "runtime's 1-second stamp collides and one leg is lost")
    st.add_argument("--binary", default="",
                    help="the built port whose runs these are (default build/bin/tomba2_port); its "
                         "mtime must be newer than every key-deciding path or the report REFUSES")
    st.add_argument("--ref-time", default="",
                    help="ISO stamp standing for the code identity a claim must be re-earned under "
                         "(default: the newest commit touching the paths that DECIDE a producer key)")
    st.add_argument("--selftest", action="store_true",
                    help="prove the check fires for a re-earned claim AND for a fossil, and that a "
                         "missing corpus REFUSES")
    args = ap.parse_args()
    return {"ingest": cmd_ingest, "report": cmd_report, "search": cmd_search,
            "show": cmd_show, "check": cmd_check, "stale": cmd_stale}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
