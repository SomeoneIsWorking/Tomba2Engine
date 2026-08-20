#!/usr/bin/env bash
# Fully automated build-and-run for the Tomba! 2 native PC port (macOS + Linux).
#
#   ./run.sh [/path/to/Tomba2.chd]
#   ./run.sh --resume [recording.pad]      continue where you left off (see below)
#
# --resume: CONTINUE FROM A PAD RECORDING instead of playing back to the spot. Every windowed run
# already records its input to scratch/bin/pad_session.pad, so --resume with no path picks up the
# last session: the game replays that input with the pacing off, the sound muted and the movies
# uncapped, then hands you the controller exactly where you stopped and keeps recording — so the
# session you end today is what --resume continues tomorrow. Give it a path to resume some other
# recording (e.g. one of replays/bugs/*.pad, to land on a filed bug).
#
# It is NOT a save state: the game really is replayed from boot, so it takes a little while for a
# long session, and it only lands where you left off if the run is deterministic. If a resume ends
# up somewhere else, that is a real port divergence and worth a bug card — the console says which
# pad frame it handed over on.
#
# Does everything end to end: builds the CHD tooling (libchdr + discdump) via CMake,
# extracts MAIN.EXE from your disc, recompiles the game core + native runtime, and launches
# it in a window. The disc image is yours to provide (never shipped) — pass it as an argument,
# or set PSXPORT_TOMBA2_DISC, or put it in a .env file, or drop a *.chd next to this script.
#
# Requirements (install once):
#   macOS:  brew install cmake sdl3 zstd zlib python3
#   Linux:  apt/dnf install clang cmake SDL3-devel libzstd-dev zlib1g-dev python3
#
# Env knobs: PSXPORT_NOAUDIO=1 (mute), PSXPORT_GPU_DUMP=dir (dump frames as PPM),
#            CC/CXX (explicit Clang paths), PSXPORT_NOWINDOW=1 (headless run).
#            PSXPORT_NOPACE=1 (run as fast as the host can). HEADLESS IS NOT UNPACED: headless
#            means no window surface and no audio device, nothing else, so a headless run paces
#            at the game's field rate exactly like a windowed one. A gate or tool that wants
#            frames rather than real time asks for NOPACE explicitly — it is the only switch
#            that means that.
# no pipefail: several steps use `cmd | head -1`, where head closing early would SIGPIPE the
# producer and (under pipefail) abort the script; results are validated explicitly instead.
set -eu
cd "$(dirname "$0")"

say() { printf '\033[1;36m[run]\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m[run] error:\033[0m %s\n' "$*" >&2; exit 1; }

# ---- 0. toolchain -------------------------------------------------------------------
command -v cmake   >/dev/null || die "cmake not found (macOS: brew install cmake)"
command -v python3 >/dev/null || die "python3 not found"
command -v pkg-config >/dev/null || die "pkg-config not found (macOS: brew install pkg-config)"
pkg-config --exists sdl3 || die "SDL3 not found (macOS: brew install sdl3; Linux: SDL3-devel / libsdl3-dev)"
CC="${CC:-clang}"
CXX="${CXX:-clang++}"
is_clang() { case "$("$1" --version 2>/dev/null)" in *clang*) return 0;; *) return 1;; esac; }
is_clang "$CC" || die "CC=$CC is not Clang"
is_clang "$CXX" || die "CXX=$CXX is not Clang"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# ---- 0a2. WHICH FRAMEWORK CHECKOUT IS THIS RUN BUILT FROM? --------------------------------------
# Default: the pinned submodule, so `git clone && ./run.sh` works standalone. Override to build
# against the workspace's framework dev clone without touching the submodule:
#
#   PSXPORT_DIR=$HOME/repo/psx/psxport ./run.sh
#
# ANNOUNCED either way, and that is the point: a binary built from in-progress framework work must
# never be mistaken for one built from the pin. Same discipline as the render-path stamp.
# external/psxport is NOT a git submodule any more (2026-08-16): it is a symlink to the workspace's
# shared framework clone when there is one — so a framework edit is live in every port at once, which
# is the point — or a private clone at psxport.pin otherwise. Establish whichever applies before we
# look at it. tools/psxport_sync.py explains the two submodule incidents that motivated the change.
python3 tools/psxport_sync.py --auto || die "could not resolve external/psxport"
PSXPORT_DIR="${PSXPORT_DIR:-external/psxport}"
[ -f "$PSXPORT_DIR/cmake/psxport.cmake" ] || die "PSXPORT_DIR=$PSXPORT_DIR is not a psxport checkout"
if [ "$PSXPORT_DIR" = "external/psxport" ]; then
  say "framework: external/psxport -> $(readlink -f external/psxport 2>/dev/null || echo '?') @ $(git -C external/psxport rev-parse --short HEAD 2>/dev/null || echo '?')$(
        [ -n "$(git -C external/psxport status --porcelain 2>/dev/null)" ] && echo ' +dirty')"
else
  say "framework: *** $PSXPORT_DIR *** (DEV CLONE $(git -C "$PSXPORT_DIR" rev-parse --short HEAD 2>/dev/null || echo '?')$(
        [ -n "$(git -C "$PSXPORT_DIR" status --porcelain 2>/dev/null)" ] && echo ' +dirty')) — NOT the recorded pin"
fi

# ---- 0b. sync git submodules (vendor/beetle-psx = the GTE/MDEC/SPU/CHD backend) -----
# A plain `git pull` does NOT update submodules, so after a pull the beetle sources can be stale and
# the link fails with undefined GTE_BindState / MDEC_*State / SPU_*State. Sync them here so
# `git pull && ./run.sh` is self-sufficient. Guard: if a submodule has UNCOMMITTED edits (the dev
# works in the beetle fork in-tree), skip the auto-checkout and just warn — never clobber local work.
# ONE implementation, shared by all three ports: external/psxport/scripts/sync-submodules.sh.
#
# The copy that used to live here was VACUOUS, not merely narrow. It guarded on
# `git -C vendor/beetle-psx status --porcelain`, but this repo has no top-level vendor/ — beetle
# lives at external/psxport/vendor/beetle-psx. So the command failed, produced an empty string,
# `[ -z "" ]` was true, and the "everything is clean, update it all" branch ran EVERY time. It
# protected nothing, while `git submodule update --recursive` happily discarded uncommitted work in
# external/psxport, which is exactly where framework changes are made. The shared script guards
# every submodule by walking them, so there is no path name to get wrong.
#
# Bootstrap: the script lives INSIDE the submodule, so on a fresh clone (or against a gitlink older
# than the script itself) it does not exist yet — init first, then call it.
if command -v git >/dev/null && [ -f .gitmodules ]; then
  if [ ! -f external/psxport/scripts/sync-submodules.sh ]; then
    say "initializing git submodules…"
    git submodule update --init --recursive || die "git submodule update failed"
  fi
  if [ -f external/psxport/scripts/sync-submodules.sh ]; then
    bash external/psxport/scripts/sync-submodules.sh || die "submodule sync failed"
  else
    say "WARNING: external/psxport/scripts/sync-submodules.sh is absent even after init —"
    say "         submodules were NOT synced and may not match this repo's recorded gitlinks."
  fi
fi

# ---- 0c. --resume [recording.pad] ----------------------------------------------------
# Consumed here so the rest of the script's positional argument (the disc) is unaffected.
RESUME_PAD=""
if [ "${1:-}" = "--resume" ]; then
  shift
  # A path that is not another option is the recording to resume; otherwise take the last session.
  if [ "${1:-}" != "" ] && [ "${1#-}" = "${1:-}" ] && [ "${1##*.}" = "pad" ]; then RESUME_PAD="$1"; shift; fi
  if [ -z "$RESUME_PAD" ]; then
    LAST=scratch/bin/pad_session.pad
    [ -f "$LAST" ] || die "--resume: no recording at $LAST yet — play a windowed session first, or pass a .pad"
    # SNAPSHOT it. Launching rotates pad_session.pad -> .1.pad before the replay source is opened, so
    # resuming the live sink directly would open a file that had just been renamed out from under it.
    RESUME_PAD=scratch/bin/pad_resume.pad
    cp "$LAST" "$RESUME_PAD"
  fi
  [ -f "$RESUME_PAD" ] || die "--resume: no such recording: $RESUME_PAD"
  say "resume: $RESUME_PAD ($(( $(wc -c < "$RESUME_PAD") / 2 )) pad frames) — fast-forwarding, then it is yours"
  export PSXPORT_PAD_RESUME="$RESUME_PAD"
fi

# ---- 1. resolve the disc ------------------------------------------------------------
DISC="${1:-${PSXPORT_TOMBA2_DISC:-}}"
if [ -z "$DISC" ] && [ -f .env ]; then
  DISC="$(sed -n 's/^[[:space:]]*PSXPORT_TOMBA2_DISC[[:space:]]*=[[:space:]]*//p' .env | head -1)"
  [ -z "$DISC" ] && DISC="$(sed -n 's/^[[:space:]]*PSXPORT_DISC[[:space:]]*=[[:space:]]*//p' .env | head -1)"
fi
if [ -z "$DISC" ]; then
  DISC="$(ls ./*.chd 2>/dev/null | head -1 || true)"
fi
[ -n "$DISC" ] && [ -f "$DISC" ] || die "no disc image — pass it as ./run.sh <disc.chd>, set PSXPORT_TOMBA2_DISC, or drop a *.chd here"
say "disc: $DISC"

# ---- 2. build the CHD tooling (libchdr + discdump) via CMake ------------------------
# ALWAYS (re)build discdump — CMake is incremental (fast when up to date), and a STALE binary is the
# macOS "not playing" trap: a discdump built before nested BIN/ path support (FindFileInTree, 2026-06-14)
# silently can't extract the BIN/*.BIN overlays, so the recomp set is built without them and fail-fasts
# (0x800810F0). The old `if [ ! -x ]` guard never rebuilt a stale binary. Don't reintroduce it.
say "building libchdr + discdump (CMake)…"
# discdump is a FRAMEWORK tool — built from the psxport submodule (external/psxport), not this game repo.
# COMPILER: clang, by USER decision (2026-08-20) and by measurement. Same workload, both binaries,
# alternating over three pairs to keep a warm page cache from crediting either one:
#   gcc    11.31  11.50  11.19 s      clang  9.67  9.53  9.70 s      -> clang 15.1% faster
# Every clang run beat every gcc run; the separation is total, not a mean that happens to differ.
# Behaviour is identical, checked against the beetle GPU oracle rather than assumed: f1120 psx path,
# 368 = 368 prims, 0 of 524,288 pixels differing.
# CC/CXX may name explicit compiler paths, but the launcher verifies that both are Clang.
CMAKE_CC_ARGS=(-DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX")

compiler_path() { command -v "$1" 2>/dev/null || printf '%s\n' "$1"; }
cmake_cache_stale() {
  local cache="$1/CMakeCache.txt" have_cc have_cxx
  [ -f "$cache" ] || return 1
  have_cc=$(sed -n 's/^CMAKE_C_COMPILER:[^=]*=//p' "$cache")
  have_cxx=$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "$cache")
  [ "$(compiler_path "$CC")" != "$(compiler_path "$have_cc")" ] ||
    [ "$(compiler_path "$CXX")" != "$(compiler_path "$have_cxx")" ]
}
cmake_configure() {
  local source="$1" build="$2"; shift 2
  local fresh=()
  if cmake_cache_stale "$build"; then
    say "$build compiler changed — refreshing CMake metadata once"
    fresh=(--fresh)
  fi
  cmake "${fresh[@]}" -S "$source" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    "${CMAKE_CC_ARGS[@]}" "$@" >/dev/null
}

cmake_configure "$PSXPORT_DIR" "$PSXPORT_DIR/build" || die "psxport cmake configure failed"
cmake --build "$PSXPORT_DIR/build" -j "$JOBS" --target discdump >/dev/null || die "discdump build failed"
DISCDUMP="$PSXPORT_DIR/build/tools/discdump"
[ -x "$DISCDUMP" ] || DISCDUMP="$PSXPORT_DIR/build/tools/discdump.exe"
[ -x "$DISCDUMP" ] || die "discdump build failed"

# ---- 3. ensure the recompiled substrate is present AND matches its input hash --------
# ONE step does all recomp provisioning: extract MAIN.EXE + the boot stub SCUS_944.54 + every overlay
# the recompiler needs (stage START/DEMO/GAME/SOP/OPN/CRD + per-area A00..A0L), run emit.py, and verify
# the generated set matches a deterministic hash of the inputs — so every machine builds byte-identical
# recomp (the area overlays MUST all be present, else a box seeds fewer resident MAIN fns and fail-fasts
# on a different miss; that determinism is exactly what the hash enforces). See tools/ensure_recomp.py.
MAIN=scratch/bin/tomba2/MAIN.EXE
mkdir -p generated scratch/bin
PSXPORT_DISCDUMP="$DISCDUMP" python3 tools/ensure_recomp.py "$DISC" || die "recomp provisioning failed"
[ -f "$MAIN" ] || die "ensure_recomp.py did not produce MAIN.EXE"

# ---- 4b. build the native port via CMake (single source of truth: cmake/tomba2_port.cmake) ----------
# CMake owns the whole port build: the source list, the vendored RmlUi static-lib subbuild + link, the
# SDL_GPU SPIR-V shader generation, the beetle/libchdr backend, and the SDL3/freetype link. It emits
# scratch/bin/tomba2_port (RUNTIME_OUTPUT_DIRECTORY). Configure is idempotent (fast when up to date); the
# build is incremental. (The old hand-rolled per-file g++ compile/link + tools/build_port.sh are retired.)
say "building the native port (CMake -j$JOBS)…"
cmake_configure . build -DPSXPORT_DIR="$(cd "$PSXPORT_DIR" && pwd)" || die "cmake configure failed"
cmake --build build -j "$JOBS" --target tomba2_port || die "port build failed"

# ---- 5. run ------------------------------------------------------------------------
say "launching Tomba! 2 (native PC port)…"
# run.sh is the user's WINDOWED entry point, so it explicitly opts into a window (PSXPORT_VK_WINDOW=1).
# The binary itself is HEADLESS by default (gpu_gpu.cpp) so agent/CI runs that forget the flag fail safe
# (no intrusive window, no pad_session.pad clobber) instead of popping a window. PSXPORT_NOWINDOW keeps
# run.sh headless.
if [ -n "${PSXPORT_NOWINDOW:-}" ]; then export PSXPORT_VK_HEADLESS=1; else export PSXPORT_VK_WINDOW=1; fi
# RmlUi debug/mod overlay assets (fonts + menu.rml) ship with the FRAMEWORK. Since the split they live
# in the psxport submodule, but the overlay disk-loads them relative to PSXPORT_ASSET_DIR (the dir that
# CONTAINS assets/). We run from the repo root, so point it at the submodule. Without this the overlay
# loads no fonts and no menu ("[rmlui] LoadDocument … FAILED").
export PSXPORT_ASSET_DIR="${PSXPORT_ASSET_DIR:-$PSXPORT_DIR}"
# Debug server ON by default so a windowed session can be inspected/driven live (tools/dbgclient.py);
# opt out with PSXPORT_DEBUG_SERVER=0. Window is windowed by default now (PSXPORT_FULLSCREEN=1 to override).
#
# The field terrain renderer 0x8002AB5C is native + ON by default (later-158). It renders PC-native
# (engine/native_terrain.cpp: float transform + real per-pixel depth, NO PSX GTE compose / packet), with
# the gameplay/scene-data prep (sway bytes + object matrix) shared with the recomp body. The later-157
# stopgap was for a now-fixed bug: the native terrain (1) read the WRONG geometry buffer (0x800A1AE8, a
# fabricated address) instead of the recomp's 0x8009FAE8, and (2) wrote the sway-angle scratch to
# scratchpad 0x1F8001C0 — a guest write the recomp never makes (it uses its own stack) — clobbering live
# engine state and breaking terrain collision (Tomba fell through). PSXPORT_TERRAIN_FAITHFUL=1 swaps in
# the GTE/packet transcription as an A/B oracle; PSXPORT_NO_TERRAIN=1 falls back to the recomp body.
PSXPORT_DEBUG_SERVER="${PSXPORT_DEBUG_SERVER:-1}" \
PSXPORT_NO_TERRAIN="${PSXPORT_NO_TERRAIN:-0}" \
PSXPORT_TOMBA2_DISC="$DISC" exec ./scratch/bin/tomba2_port "$MAIN"
