---
name: bug-tracker
description: The in-repo kanban for USER-reported bugs (tools/kanban.py over docs/kanban/ cards). Use when the user reports a symptom ("X is broken", "Y softlocks", a bug dump mid-work), when you start chasing one, and when you confirm a fix. Also the first stop for "what bugs are open".
---

# Bug tracker — `tools/kanban.py`

Local, greppable, in-repo. Columns: `backlog | todo | doing | done`. Cards in `docs/kanban/`.

```
python3 tools/kanban.py list                 # the board
python3 tools/kanban.py show <id>
python3 tools/kanban.py add "<symptom as the USER said it>" --label bug --label pc-skip
python3 tools/kanban.py move <id> doing      # when you start chasing it
python3 tools/kanban.py note <id> "<what you ruled out / measured>"
python3 tools/kanban.py evidence <id> <path> # screenshots -> docs/reference/issues/
python3 tools/kanban.py search <words>
```

## The loop

1. **User reports a symptom → file it immediately and keep working.** Do not pivot to investigate
   each item in a bug dump; capture them so nothing is lost, then finish what you're on.
2. **Title the card with the user's own observation**, not your theory. The theory changes; the
   observation is the ground truth you'll re-check against.
3. `move <id> doing` when you actually start. Record **dead ends** as notes as you go — a ruled-out
   cause is worth as much to the next session as the fix.
4. On a confirmed fix: promote the durable knowledge to `docs/findings/<subsystem>.md`
   (symptom / status / cause / fix / refs), regenerate with `tools/findings.py`, then
   `move <id> done` — in the same commit as the code.

## Before investigating anything

`python3 tools/findings.py <symptom words>` FIRST — it searches the curated registry plus the raw
journal. "no match" is itself information (this is new ground). This is the read half of
read-then-write; skipping it is how a solved bug gets re-walked.

Related: `tools/portmap.py` (is it ported / is it a hack), `tools/parity.py` (is it SBS byte-exact),
`tools/behavior.py` (is this divergence deliberate), `tools/codemap.py --addr <hex>` (who owns a
guest address). See CLAUDE.md § the tracking stack.
