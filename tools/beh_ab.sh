#!/usr/bin/env bash
# tools/beh_ab.sh — the beh_* rebuild verification harness (kanban #10).
#
# WHY: a native `beh_*` body is a REBUILD of a guest behaviour handler — it matches the RESULT, not
# the PSX instruction stream — so `port_check` (a static store-sequence comparison) cannot gate it.
# Two shipped user-visible bugs were one-instruction semantic slips in such a body (kanban #2
# prng_velocity_machine's hardcoded 3 from a delay slot; #1/#30 proximityCheck's sign- vs zero-extend).
# The only gate that finds that class is DIFFERENTIAL: same deterministic scenario, once with the
# native body and once with the substrate `gen_func_*` body, guest state compared.
#
# THE INSTRUMENT — read this before using another one:
#   `PSXPORT_MIRROR_VERIFY=<addr>` (the strict per-invocation mirror) is NOT a valid gate for a
#   rebuilt beh_* body. Measured 2026-07-23 on replays/bugs/bucket-softlock.pad: with `ctl` forcing
#   BOTH legs onto the substrate for the armed handler AND every other beh_* off the native table,
#   the gate still reported 37167 mismatches for 0x80124E74 alone (hi/lo left by a multiply, plus
#   ~14k object-field bytes at 0x800Fxxxx). Cause: leg 1 runs with the OVERRIDE REGISTRY live, leg 2
#   sets inSubstrateLeg and suppresses it, so every nested native engine override differs between the
#   legs — the noise floor is the whole nested call chain, not the handler. A green MIRROR_VERIFY on
#   a beh_* therefore means nothing and a red one is unattributable.
#   The valid instrument is END-STATE comparison at a rendezvous: run the WHOLE scenario twice, with
#   only the handler under test swapped (PSXPORT_BEH_SUBSTRATE=<addr>), and diff the 2 MB RAM dump
#   PLUS the .spad scratchpad sidecar (main RAM is BLIND to scratchpad — a native once clobbered
#   0x1F8001C0 and broke collision). Nested overrides are native in BOTH runs, so they cancel.
#
# Subcommands, in the order you must use them:
#   cov   <replay.pad> <frames>          — COVERAGE first. Which handlers does this scenario actually
#                                          dispatch natively (PSXPORT_DEBUG=behcov)? A body the replay
#                                          never reaches cannot be A/B'd by it — an A/B that never
#                                          enters the body proves nothing.
#   base  <replay.pad> <frames>          — the reference end-state dump (all natives installed).
#   one   <replay.pad> <frames> <addr>   — the same run with ONE handler forced to its gen body, then
#                                          cmp against the base dump. 0 differing bytes in RAM and
#                                          spad = the rebuild is equivalent on the path this scenario
#                                          executes. Any diff = a divergence to root-cause.
#   sweep <replay.pad> <frames> <file>   — `one` for every address in <file> (one bare hex per line).
#
# Artifacts: scratch/logs/beh_ab/ + scratch/bin/beh_ab/ (never /tmp).
set -eu
cd "$(dirname "$0")/.."
mode="${1:?usage: beh_ab.sh cov|base|one|sweep <replay.pad> <frames> [addr|addrfile]}"
pad="${2:?replay .pad path}"
frames="${3:?frame count to run}"
log=scratch/logs/beh_ab
dump=scratch/bin/beh_ab
mkdir -p "$log" "$dump"
tag="$(basename "$pad" .pad)"

# One replay run. $1 = PSXPORT_BEH_SUBSTRATE value ("" = all natives live), $2 = dump path, $3 = log.
run_once() {
  printf 'run %s\ndumpram %s\nquit\n' "$frames" "$2" \
    | env PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 "PSXPORT_PAD_REPLAY=$pad" \
      "PSXPORT_BEH_SUBSTRATE=$1" ./scratch/bin/tomba2_port > "$3" 2>&1
}

# cmp base vs forced, RAM + scratchpad. Prints "<addr> OK" or the first differing bytes.
compare() {
  local addr="$1" b="$dump/base_$tag.bin" f="$dump/forced_${tag}_$1.bin" nram nspad
  nram=$(cmp -l "$b" "$f" 2>/dev/null | wc -l)
  nspad=$(cmp -l "$b.spad" "$f.spad" 2>/dev/null | wc -l)
  if [ "$nram" = 0 ] && [ "$nspad" = 0 ]; then
    echo "$addr OK ram=0 spad=0"
  else
    echo "$addr DIFF ram=$nram spad=$nspad"
    cmp -l "$b" "$f" 2>/dev/null | head -12
  fi
}

case "$mode" in
  cov)
    l="$log/cov_$tag.log"
    printf 'run %s\nquit\n' "$frames" \
      | env PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 "PSXPORT_PAD_REPLAY=$pad" \
        PSXPORT_DEBUG=behcov ./scratch/bin/tomba2_port > "$l" 2>&1 || true
    echo "log: $l"
    grep '^\[behcov\]' "$l" | awk '{print $2, $3}' | sort -u
    ;;
  base)
    run_once "" "$dump/base_$tag.bin" "$log/base_$tag.log"
    echo "base: $dump/base_$tag.bin"
    ;;
  one)
    addr="${4:?bare-hex handler address, e.g. 80117658}"
    run_once "$addr" "$dump/forced_${tag}_$addr.bin" "$log/forced_${tag}_$addr.log"
    compare "$addr"
    ;;
  sweep)
    file="${4:?file of bare-hex addresses, one per line}"
    while read -r addr; do
      [ -n "$addr" ] || continue
      run_once "$addr" "$dump/forced_${tag}_$addr.bin" "$log/forced_${tag}_$addr.log"
      compare "$addr"
    done < "$file"
    ;;
  *) echo "unknown mode $mode" >&2; exit 2 ;;
esac
