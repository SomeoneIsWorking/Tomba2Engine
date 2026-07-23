#!/usr/bin/env bash
# tools/precommit_gate.sh — the pre-commit gate. Install once with:
#     ln -sf ../../tools/precommit_gate.sh .git/hooks/pre-commit
#
# WHY THIS EXISTS. A machine-specific path or a copyrighted blob is trivial to
# introduce and expensive to remove: once it is in a commit, taking it out means
# rewriting history. The cheap moment to catch it is BEFORE the commit exists.
#
# It has already caught a real leak: filing a kanban card, a `w` in a backtick
# inside a DOUBLE-QUOTED shell string was command-substituted, and the real `w`
# output — username and login session — was baked into the card body and
# committed. Nothing noticed until a full-history audit ran days later.
#
# Checks, in order (fail fast, cheapest first):
#   1. go_public.py scan --current  — machine paths / copyrighted blobs / doc
#      references to private ignored data, over the working tree + HEAD.
#   2. codemap.py --dup-installs    — two files installing an override on ONE
#      guest address. The runtime guard aborts on this at boot; catching it here
#      turns a runtime abort into a commit-time message.
#
# Both are fast (seconds). Skip deliberately with `git commit --no-verify` when
# you know better — but say why in the commit message.
set -uo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 0
fail=0

say() { printf '\033[1;36m[pre-commit] %s\033[0m\n' "$*"; }
bad() { printf '\033[1;31m[pre-commit] %s\033[0m\n' "$*"; }

if [ -f tools/go_public.py ]; then
  if ! out="$(python3 tools/go_public.py scan --current 2>&1)"; then
    bad "publication audit FAILED — a blocking leak is about to be committed:"
    printf '%s\n' "$out" | grep -A4 -E "BLOCKING" | head -20
    bad "fix the file (repo-relative path / env var / <HOME> placeholder), or --no-verify if this is a false positive."
    fail=1
  else
    say "publication audit clean"
  fi
fi

if [ -f tools/info.py ]; then
  out="$(python3 tools/info.py check 2>&1)"
  case "$out" in
    *DISTRUSTED*|*FALSIFIED*) bad "information system flags:"; printf '%s\n' "$out" | sed 's/^/  /' ;;
    *) say "no distrusted instruments, no falsified claims" ;;
  esac
fi

if [ -f tools/codemap.py ]; then
  dup="$(python3 tools/codemap.py --dup-installs 2>/dev/null | tail -1)"
  case "$dup" in
    0\ address*) say "override ownership clean (0 double-installs)" ;;
    *)           bad "duplicate override ownership: $dup"
                 bad "two files installing one guest address — the runtime guard aborts on this. Give the address ONE owner."
                 fail=1 ;;
  esac
fi

exit "$fail"
