#!/usr/bin/env bash
# ab_replay.sh <binary> <port> <tag> <replay.pad> <target-frame>
# Same deterministic same-frame A/B as ab_leg.sh, but driven by a recorded pad replay instead of
# AUTO_SKIP, so it can reach an effect AUTO_SKIP never triggers (the water jet, the impact burst).
set -eu
BIN=$1; PORT=$2; TAG=$3; PAD=$4; TARGET=$5
cd "$(dirname "$0")/.."
D="python3 external/psxport/tools/dbgclient.py --port $PORT"
LOG=scratch/logs/ab_$TAG.log
rm -f "$LOG"
setsid nohup env PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=$PORT \
  PSXPORT_PAD_REPLAY="$PAD" "$BIN" scratch/bin/tomba2/MAIN.EXE > "$LOG" 2>&1 < /dev/null &
PID=$!
echo "$TAG pid=$PID replay=$PAD"
for i in $(seq 1 120); do sleep 1; $D frame >/dev/null 2>&1 && break; done
$D pause
NOW=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/')
echo "$TAG paused at $NOW, target $TARGET"
if [ "$NOW" -gt "$TARGET" ]; then echo "$TAG FATAL: overshot ($NOW > $TARGET)"; kill $PID; exit 1; fi
$D step $((TARGET - NOW)) >/dev/null
for i in $(seq 1 900); do
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
