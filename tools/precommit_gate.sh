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
#   3. codemap.py --selftest        — the codemap can still answer POSITIVELY for
#      every ownership shape it claims to cover. CLAUDE.md sends every agent to
#      `--addr <hex>` BEFORE reimplementing a FUN_xxxx, so a scanner that quietly
#      stops seeing a shape does not fail loudly — it answers "NO native owner
#      found" and causes a duplicated port. 26 addresses were in exactly that
#      state on 2026-08-05. A self-test nobody runs is the same bug one level up,
#      so it runs here.
#
# Both are fast (seconds). Skip deliberately with `git commit --no-verify` when
# you know better — but say why in the commit message.
set -uo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT" || exit 0
fail=0

say() { printf '\033[1;36m[pre-commit] %s\033[0m\n' "$*"; }
bad() { printf '\033[1;31m[pre-commit] %s\033[0m\n' "$*"; }

if [ -f tools/check_cpp_style.py ]; then
  if python3 tools/check_cpp_style.py; then
    say "C++ formatting and ownership caps clean"
  else
    bad "C++ formatting/ownership check failed"
    fail=1
  fi
fi

# The framework you BUILT against must be the one psxport.pin RECORDS, or a fresh clone builds
# something you never tested. This tree shipped exactly that once (a pin whose GameHooks lacked a field
# the game used, so a bare clone did not compile). tools/psxport_sync.py --check asserts nothing and
# says so when the tree was never configured, rather than passing quietly.
if [ -f tools/psxport_sync.py ]; then
  if python3 tools/psxport_sync.py --check; then
    say "psxport pin matches the framework this tree was built against"
  else
    bad "psxport pin does NOT match the framework this tree was built against"
    fail=1
  fi
fi

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

  if out="$(python3 tools/codemap.py --selftest 2>&1)"; then
    say "codemap selftest: $(printf '%s\n' "$out" | tail -1)"
  else
    bad "codemap SELFTEST FAILED — the index has stopped resolving an ownership shape it claims to cover:"
    printf '%s\n' "$out" | grep -E '^(FAIL|  \[FAIL)' | sed 's/^/  /'
    bad "every agent runs 'codemap.py --addr <hex>' before porting a FUN_xxxx; a silent miss there causes a duplicated port."
    fail=1
  fi
fi

exit "$fail"
