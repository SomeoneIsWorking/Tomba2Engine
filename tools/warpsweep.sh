#!/bin/bash
# warpsweep.sh — per-area pc_render vs psx_render reference sweep.
#
# WHY THIS EXISTS IN tools/ (it was an untracked scratch script, and it was WRONG): the scratch version
# took its "psx reference" from a BARE `renderpsx` REPL command, which at the time only PRINTED the flag
# and set nothing. Its three shots A(f)/B(f+1)/C(f+2) were therefore ALL pc_render frames, so the sweep
# compared pc against pc for its entire run and carried ZERO information about psx_render. In a panning
# scene that manufactures a fake "displacement" bug — kanban #26 (area 12 ceiling band) was exactly that.
# See docs/findings/render.md ("was a broken instrument, not a render bug").
#
# THE RULES THIS SCRIPT ENCODES:
#   1. Never compare adjacent frames. The pad replay is bit-deterministic, so the psx reference is a
#      SECOND RUN at the SAME frame index with PSXPORT_RENDER_PSX=1 set AT BOOT.
#   2. This script still takes its reference from a SECOND BOOT RUN, and that stays the protocol — but
#      the reason recorded here was WRONG and is corrected (kanban #41, 2026-08-05). The `renderpsx`
#      toggle is NOT "honoured per scene entry": it is honoured every frame. Two real effects were being
#      read as one latch. (a) A `shot` issued after `run 1` reads the PREVIOUSLY presented frame, because
#      the REPL executes commands before the next frame runs — so a toggle needs `run 2`, not `run 1`.
#      (b) Switching OUT of psx_render mid-scene used to leave the picture permanently near-black: the
#      area-cache trust latches only ticked on the pc_render branch. That is FIXED (drawOTag now ticks
#      Render::areaCacheTrustTick before the mode branch), and a mid-scene toggle in either direction is
#      now bit-identical to the corresponding boot-time reference — verified 0/76800 px, area 0 only.
#      A boot-time reference is still preferred here: it needs no in-band toggle and no lag accounting.
#   3. One process per area per leg. Chained warps drift (kanban #36 cold-warp self-destruct,
#      #37 areas 16/17/18 hang), and a hung area must not poison its neighbours.
#   4. The comparison is `tools/render_cmp.py diff` — which really diffs two image paths. Its exit
#      status and its printed count are both checked; a missing frame is reported, never swallowed.
#
# usage: tools/warpsweep.sh <tag> [area...]        # areas are 0..21 (the game has 22; see docs/areas.md)
#   out: scratch/screenshots/warpsweep/<tag><area>_{pc,psx}.png, <tag><area>.{pc,psx}.log, <tag>.report

set -u
tag=${1:?usage: warpsweep.sh <tag> [area...]}; shift
areas=("$@")
[ ${#areas[@]} -eq 0 ] && areas=($(seq 0 21))

out=scratch/screenshots/warpsweep
mkdir -p "$out"
report="$out/$tag.report"
: > "$report"

REPLAY=${WARPSWEEP_REPLAY:-replays/bugs/seesaw-weight.pad}
SETTLE=${WARPSWEEP_SETTLE:-3000}   # frames of replay before warping (a cold warp needs a settled game)
DWELL=${WARPSWEEP_DWELL:-600}      # frames after the warp (a cold warp renders black for ~45)
TIMEOUT=${WARPSWEEP_TIMEOUT:-600}  # seconds per leg-run
# Pin the binary. A sweep is long and someone else's rebuild lands in scratch/bin mid-run, which would
# put the two legs on two different builds. Copy the binary aside and point WARPSWEEP_BIN at it.
BIN=${WARPSWEEP_BIN:-./scratch/bin/tomba2_port}

leg() {                            # leg <area> <suffix> [env...]
  local a=$1 suffix=$2; shift 2
  printf 'newgame\nrun %s\nwarp %s\nrun %s\nstage\nshot %s\nquit\n' \
    "$SETTLE" "$a" "$DWELL" "$out/${tag}${a}_${suffix}.png" \
  | timeout "$TIMEOUT" env "$@" PSXPORT_REPL=1 PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 \
      PSXPORT_PAD_REPLAY="$REPLAY" "$BIN" > "$out/${tag}${a}.${suffix}.log" 2>&1
  echo $?
}

for a in "${areas[@]}"; do
  if [ "$a" -lt 0 ] || [ "$a" -gt 21 ]; then
    echo "area $a: SKIPPED — not an area (the game has 22, 0..21; see docs/areas.md)" | tee -a "$report"
    continue
  fi
  rc_pc=$(leg "$a" pc)
  rc_psx=$(leg "$a" psx PSXPORT_RENDER_PSX=1)
  line=$(python3 tools/render_cmp.py diff "$out/${tag}${a}_pc.png" "$out/${tag}${a}_psx.png" | tail -1)
  rc_cmp=$?
  printf 'area %-3s pc_exit=%-3s psx_exit=%-3s cmp_exit=%-3s %s\n' \
    "$a" "$rc_pc" "$rc_psx" "$rc_cmp" "$line" | tee -a "$report"
  cp scratch/screenshots/cmp_triptych.png "$out/${tag}${a}_triptych.png" 2>/dev/null
done

echo "report: $report"
