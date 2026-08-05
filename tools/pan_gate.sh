#!/usr/bin/env bash
# pan_gate.sh <binary> <port> <tag> <pan|still>
# THE TAP GATE. A tap recovers a transform from engine state the camera has already been folded into,
# so its residue is a FUNCTION OF THE CAMERA: the object jitters while the camera pans and is quiet
# while it is still. This drives the seaside free-roam to a settled state, then dumps PRESENTED frames
# (fps60 interleaves real and interpolated ones — the only headless view of a temporal artefact) with
# `debug preseqobj` on, either while the camera is panning or while it is still.
# Feed the log to tools/preseqobj_check.py --node <billboard node> --node <control>.
set -eu
BIN=$1; PORT=$2; TAG=$3; MODE=$4
cd "$(dirname "$0")/.."
D="python3 external/psxport/tools/dbgclient.py --port $PORT"
LOG=scratch/logs/pan_$TAG.log
rm -f "$LOG"
setsid nohup env PSXPORT_VK_HEADLESS=1 PSXPORT_NOAUDIO=1 PSXPORT_DEBUG_SERVER=$PORT \
  PSXPORT_AUTO_SKIP=1 "$BIN" scratch/bin/tomba2/MAIN.EXE > "$LOG" 2>&1 < /dev/null &
PID=$!
for i in $(seq 1 120); do sleep 1; $D frame >/dev/null 2>&1 && break; done
$D pause >/dev/null
N=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/')
TARGET=$((N + 60))          # settle past the arrival transition before anything is measured
$D step 60 >/dev/null
for i in $(seq 1 300); do F=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/'); [ "$F" -ge "$TARGET" ] && break; sleep 0.5; done
[ "$F" -ge "$TARGET" ] || { echo "$TAG FATAL: never settled"; kill $PID; exit 1; }
$D debug preseqobj >/dev/null
if [ "$MODE" = pan ]; then $D press right >/dev/null; $D step 4 >/dev/null; sleep 1; fi
$D preseq 48 "scratch/screenshots/pan_${TAG}" >/dev/null
$D step 48 >/dev/null
for i in $(seq 1 240); do F2=$($D frame | sed 's/.*frame=\([0-9]*\).*/\1/'); [ "$F2" -ge $((F + 48)) ] && break; sleep 0.5; done
[ "$MODE" = pan ] && $D release right >/dev/null
echo "$TAG ($MODE): frames $F..$F2, preseqobj records $(grep -c '\[preseqobj\]' "$LOG")"
$D quit >/dev/null 2>&1 || true
sleep 2
kill $PID 2>/dev/null || true
