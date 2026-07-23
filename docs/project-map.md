
## Publication cleanliness gate (2026-07-23)

`git push` publishes the FULL HISTORY, so a machine path or copyrighted blob committed once is
expensive to remove later — it means rewriting history. Two tools keep the repo publishable:

- **`tools/go_public.py`** — audits (A) copyrighted/oversized blobs, (B) machine-specific paths
  (`/home/<user>`, `~/`, usernames), (C) committed docs referencing private gitignored data.
  `scan --current` = working tree + HEAD (fast); `scan` = full history; `rules -o replace.txt`
  emits a `git filter-repo --replace-text` mapping when history does need scrubbing.
  Vendored into `tools/` deliberately so the check travels with the repo.
- **`tools/precommit_gate.sh`** — the pre-commit hook. Runs the `--current` audit plus
  `codemap.py --dup-installs`. Install once:
  `ln -sf ../../tools/precommit_gate.sh .git/hooks/pre-commit`

**Remediation order when history is dirty: scrub, don't wipe.** `git filter-repo` rewrites hashes
but PRESERVES every commit and message; squashing to one orphan commit destroys them and is a last
resort only when scrubbing is genuinely intractable.

`tools/kanban.py` also refuses card text carrying machine-identifying data. That is not
hypothetical: a card body written with backticks inside a DOUBLE-quoted shell string had `` `w` ``
command-substituted, baking a real username/tty line into a committed file. Put long card bodies in
a file and pass `--body "$(cat f.txt)"`, or single-quote them.
