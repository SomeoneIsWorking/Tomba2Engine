# The GRAPHICS PRODUCER DB — what draws this game, and who owns each of it

One file per producer, created **mechanically** the first time that producer submits geometry in a run.
Tooling: `tools/producers.py` (`ingest` / `report` / `report --todo` / `search` / `show` / `check`).
Design and staging: `external/psxport/docs/plans/graphics-producer-db.md`.

## The question this answers, that nothing else does

`docs/code-map.md` says what code exists. `docs/findings/` + `docs/kanban/` say what has been tried.
psxport's `overrides::coverage` says how much of what we *own* a run reached — a fraction whose
denominator is our own ambition. This is the opposite fraction: **of the picture-producing work the game
actually DID, how much does the PC produce natively?** It can only be discovered by running.

## How a row appears

The port keeps a per-Core census (`external/psxport/runtime/psx/producer_census.h`) keyed by the
**guest submitter fn address** — the same key space as `tomba::native::declareOverride`, which is what makes "does
this effect have a native producer" a *derived* fact instead of a box someone must remember to tick. It
appends observations to `scratch/producers/run-<stamp>.jsonl`; `tools/producers.py ingest` folds them
into this directory, and `run.sh` calls it, so **ordinary play populates the DB**.

## The two halves of the frontmatter, and why they never mix

`ingest` writes only the `observed` half and preserves everything else verbatim.

| observed (machine-owned) | meaning |
|---|---|
| `first_seen` / `last_seen` / `runs` | when it was first met, last met, how many ingested runs saw it |
| `prims_guest_max` / `prims_native_max` | peak prims attributed per leg — a size, for ranking work |
| `frames_seen`, `gte_calls_max` | how constant it is; its GTE cost |
| `layers`, `sub_signatures` | union of RQ layers and material signatures it drew with |
| `has_native`, `native_reached` | **DERIVED from the override table** — never hand-edited |

| curated (yours) | meaning |
|---|---|
| `name` | what the effect IS ("radial plume", "title logo sprites") |
| `re_status` | `unknown` → `decompiled` → `re-verified` → `ported-partial` → `ported` |
| `re_evidence` | the claim / issue / decomp that justifies `re_status`. Required above `unknown` |
| `producer_file` | the native producer, e.g. `game/render/fx_plume.cpp` |
| `partial_because` | **required** when `ported-partial`: the branch that is NOT ported |
| `compare_status` | `never-compared` / `pixel-diffed` / `byte-exact` / `diverges` |
| `notes` | links to `[[claims]]`, kanban cards, issues |

Only slow-moving observed fields are tracked (maxima and first/last, never per-run counters), so after a
few sessions a play run's `git diff` is **new producers and nothing else**.

## Reading a negative from this DB

Every number here is bounded by what the ingested runs PLAYED, and by how much of each frame the census
could attribute at all. So:

* **A producer absent from this directory is NOT-OBSERVED, never absent.**
* `report` prints the blindness caveats every time, on purpose. The per-run attribution denominators
  (`prims_seen`, `prims_attributed`, `gp0_anon`, `span_miss`, `unscoped_native`, `overflow`) are in the
  `totals` record of each `scratch/producers/*.jsonl` and in the `ingest` output. **Read them before
  quoting any coverage percentage.**
* `has_native: true` with `native_reached: false` means we think we own it and its body never ran. That
  is a lie in the DB, and `report --todo` ranks it above all new work.
* `ingest` **exits non-zero and refuses** when there is no observation directory, rather than reporting
  "0 new producers" — scanning nothing must never look like finding nothing.

## One row, one guest fn — and when to split

A fn that is honestly two effects is split BY HAND, and the split records the discriminator it used
(`sub_signatures` is the evidence for when that is needed). The known case is Tomba!2's plume producer
`FUN_8002BC9C`, whose subtype `0x14`/`0x15` reaches a different emitter family with no port — a single
row there would report a native producer over a branch that has none.
