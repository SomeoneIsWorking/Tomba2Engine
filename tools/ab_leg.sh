#!/usr/bin/env bash
# ab_leg.sh <binary> <port> <tag> <target-frame>
# Launches one A/B leg headless, PAUSES it as early as the debug server allows, steps to an
# EXACT absolute frame, and captures the picture + one preseqobj-instrumented present sequence.
# Same frame in every leg is the point: without it a prim-count difference is indistinguishable
# from the two runs having sampled different animation phases.
set -eu
BIN=$1; PORT=$2; TAG=$3; TARGET=$4
cd "$(dirname "$0")/.."
D="python3 external/psxport/tools/dbgclient.py --port $PORT"
LOG=scratch/logs/ab_$TAG.log
rm -f "$LOG"
# PSXPORT_NOPACE=1 — as fast as the host can. Headless is PACED like a windowed run now
# (they are one program; the pacer used to early-return when there was no window), so a
# tool that wants frames rather than real time has to ask for the unpaced run explicitly.
setsid nohup env PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_NOPACE=1 PSXPORT_DEBUG_SERVER=$PORT \
  PSXPORT_AUTO_SKIP=1 "$BIN" scratch/bin/tomba2/MAIN.EXE > "$LOG" 2>&1 < /dev/null &
PID=$!
echo "$TAG pid=$PID"
for i in $(seq 1 120); do sleep 1; $D frame >/dev/null 2>&1 && break; done
$D pause
NOW=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/')
echo "$TAG paused at $NOW, target $TARGET"
if [ "$NOW" -gt "$TARGET" ]; then echo "$TAG FATAL: overshot target ($NOW > $TARGET) — raise TARGET"; kill $PID; exit 1; fi
# `step` returns immediately and the frames run asynchronously, so POLL until the counter actually
# arrives. Without this the leg captures whatever frame it happened to be on — the exact defect this
# script exists to prevent.
$D step $((TARGET - NOW)) >/dev/null
for i in $(seq 1 600); do
  F=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/')
  [ "$F" -ge "$TARGET" ] && break
  sleep 0.5
done
[ "$F" -ge "$TARGET" ] || { echo "$TAG FATAL: never reached $TARGET (stuck at $F)"; kill $PID; exit 1; }
echo "$TAG now at frame=$F"
$D debug preseqobj
$D preseq 24 "scratch/screenshots/ab_${TAG}_preseq"
$D step 24 >/dev/null
for i in $(seq 1 120); do
  F2=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/')
  [ "$F2" -ge $((TARGET + 24)) ] && break
  sleep 0.5
done
echo "$TAG captured through frame=$F2"
$D shot "scratch/screenshots/ab_${TAG}.png"
$D quit >/dev/null 2>&1 || true
sleep 2
kill $PID 2>/dev/null || true
echo "$TAG done"
