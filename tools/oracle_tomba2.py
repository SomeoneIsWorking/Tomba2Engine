#!/usr/bin/env python3
"""Tomba! 2's side of the oracle comparison: checkpoint predicates, declared state, exclusions and
the per-core input policy (psxport docs/oracle.md: the framework owns the barrier mechanism, the
title owns what is compared and how each core is driven toward the next checkpoint).

Every predicate reads main RAM only, because the console reference cannot read the scratchpad.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Optional, Sequence

from oracle_cores import CoreError, CoreSession

STAGE_GAME = 0x8010637C          # task 0 entry while the GAME stage runs
TASK0 = 0x801FE000               # task table entry 0 (state u16 at +0, stage entry u32 at +0x0C)
TASK0_ENTRY = TASK0 + 0x0C
STATE_MACHINE = 0x801FE048       # sm[0x48..0x52]: six halfwords (outer 48/4a/4c, leaf 4e/50/52)
AREA_INDEX = 0x800BF870
PLAYER_G = 0x800E7E80            # Tomba's master G block (0x184 bytes, game/player/actor_tomba.h)
DWELL_COUNTER = 0x800E809C       # VBlanks since the main loop last passed its vblank gate (GameConfig)
PAD_CURRENT = 0x800ECF54         # frame fence FUN_800788AC: this frame's pad word (docs/engine_re.md)
PAD_PRESSED = 0x800E7E68         # ... buttons newly pressed this frame
PAD_RELEASED = 0x800F23A4        # ... buttons newly released this frame
TASK_RUNNING = 4                 # task slot state while the scheduler is inside that task's logic


@dataclass(frozen=True)
class DeclaredRange:
    name: str
    address: int
    size: int
    decisive: bool  # a decisive mismatch is a divergence; an informational one is reported only

    @property
    def end(self) -> int:
        return self.address + self.size


DECLARED = (
    DeclaredRange("task0.state", TASK0, 2, False),  # frame-phase dependent, see EXCLUDED
    DeclaredRange("task0.entry", TASK0_ENTRY, 4, True),
    DeclaredRange("state_machine", STATE_MACHINE, 12, True),
    DeclaredRange("area_index", AREA_INDEX, 1, True),
    DeclaredRange("player.position", PLAYER_G + 0x2C, 12, True),   # 16.16 X/Y/Z
    DeclaredRange("player.motion", PLAYER_G + 0x44, 10, True),     # dir/speed pairs
    DeclaredRange("player.G", PLAYER_G, 0x184, False),
    # The pad words are what each core's game actually read; a mismatch there is an input-delivery
    # defect in this harness, not a product divergence, and must fail before anything downstream.
    DeclaredRange("pad.current", PAD_CURRENT, 4, True),
    DeclaredRange("pad.pressed", PAD_PRESSED, 4, True),
    DeclaredRange("pad.released", PAD_RELEASED, 4, True),
)

EXCLUDED = {
    "task0 +0x00 scheduler state (informational only)":
        "the product samples after the buffer swap, when FUN_800506D0 has re-armed the yielded task "
        "to 2; the console samples at the VBlank interrupt while the guest still spins in its "
        "vblank gate before that sweep, so it reads 1 (docs/engine_re.md main loop order)",
    "task0 +0x04..+0x0B (BIOS thread/event handles)":
        "assigned by the thread service; psxport's native HLE threads number them differently "
        "from the SCPH-1001 BIOS (docs/engine_re.md task scheduler)",
    "scratchpad 0x1F800000..0x1F800400": "the console reference exposes main RAM only",
    "PRNG, callback ring, stdio, OT and packet pools":
        "allocation/timing state, not gameplay state (docs/oracle.md retained observations)",
    "VRAM, SPU and CD device state": "separate pixel, audio and hardware questions",
}

# Representative free-roam input after the third checkpoint: held buttons and game-frame counts.
# The first segment repeats the released pad the checkpoint parked with (Playback's precondition).
# Tomba walks left away from the landing spot, back right (short of the fisherman, whose dialogue
# would take the pad), then jumps standing and while walking.
GAMEPLAY = (
    (frozenset(), 30),
    (frozenset({"left"}), 60),
    (frozenset(), 30),
    (frozenset({"right"}), 90),
    (frozenset(), 30),
    (frozenset({"cross"}), 6),
    (frozenset(), 60),
    (frozenset({"left", "cross"}), 6),
    (frozenset({"left"}), 60),
    (frozenset(), 30),
)


class Unreached(CoreError):
    """A checkpoint predicate never held within its frame budget."""


def _u32(core: CoreSession, address: int) -> int:
    return int.from_bytes(core.read(address, 4), "little")


def state_machine_view(raw: bytes) -> dict[str, int]:
    names = ("48", "4a", "4c", "4e", "50", "52")
    return {name: int.from_bytes(raw[i * 2:i * 2 + 2], "little") for i, name in enumerate(names)}


@dataclass(frozen=True)
class Observation:
    """What one frame of main RAM says about progress."""
    stage_entry: int
    sm: dict[str, int]

    @property
    def in_game_stage(self) -> bool:
        return self.stage_entry == STAGE_GAME

    @property
    def in_field(self) -> bool:
        """The field runs: outer machine sm[0x4a]==1 with the area machine at sm[0x4c]==2."""
        return self.in_game_stage and self.sm["4a"] == 1 and self.sm["4c"] == 2

    @property
    def in_free_roam(self) -> bool:
        """The player has the pad: the field's leaf machine sm[0x4e] is 1. The opening cutscene
        (Tomba's landing and the fisherman's dialogue) runs it at 9 and ignores the pad."""
        return self.in_field and self.sm["4e"] == 1


def advance(core: CoreSession, frames: int, strict: bool = False) -> None:
    """Step `frames` Tomba! 2 game frames. The product's driver runs one game frame per step.

    The VBlank-stepped console runs a game frame when the guest main loop passes its vblank gate
    (the dwell counter resets; the field gates on 2 VBlanks). The barrier is pinned one VBlank
    after that gate: task 0's logic has finished and the guest spins in the gate. The pad polled
    at that non-gate VBlank is what the next frame's pad fence reads, so a hold set at the
    barrier reaches the game exactly one frame later on the console (see Playback). `strict`
    refuses a frame whose logic overran a second gate (boot loaders legitimately run across
    frames; scheduled gameplay must not)."""
    if not core.vblank_stepped:
        core.step(frames)
        return
    for _ in range(frames):
        dwell = _u32(core, DWELL_COUNTER)
        while True:
            core.step(1)
            previous, dwell = dwell, _u32(core, DWELL_COUNTER)
            if dwell < previous:
                break
        stepped_past_gate = 0
        while stepped_past_gate == 0 or int.from_bytes(core.read(TASK0, 2), "little") == TASK_RUNNING:
            core.step(1)
            stepped_past_gate += 1
            previous, dwell = dwell, _u32(core, DWELL_COUNTER)
            if strict and dwell < previous:
                raise CoreError(f"{core.name}: the guest passed its vblank gate again after "
                                f"{stepped_past_gate} VBlank(s) with task 0 still running; a lag or "
                                f"1-VBlank frame cannot be aligned with the product's frame count")


class Playback:
    """Feeds one core a known per-frame button schedule so that game frame K's pad fence reads
    schedule[K] on both core kinds. The native product reads a hold in the frame it is stepped;
    the console reads it one frame later (advance), so the console is driven one hold ahead. The
    core must be parked with schedule[0] already held (checkpoint arrival releases the pad)."""

    def __init__(self, core: CoreSession, schedule: Sequence[frozenset[str]]):
        if not schedule:
            raise ValueError("empty schedule")
        if core.held != schedule[0]:
            raise CoreError(f"{core.name}: parked holding {sorted(core.held)} but the schedule starts "
                            f"with {sorted(schedule[0])}; the first scheduled frame must repeat the parked pad")
        self.core = core
        self.schedule = schedule
        self.frame = 0

    def step(self) -> None:
        """Run the next scheduled game frame."""
        if self.frame >= len(self.schedule):
            raise IndexError("schedule exhausted")
        if self.core.vblank_stepped:
            following = self.schedule[min(self.frame + 1, len(self.schedule) - 1)]
            self.core.hold(following)
        else:
            self.core.hold(self.schedule[self.frame])
        advance(self.core, 1, strict=True)
        self.frame += 1


def observe(core: CoreSession) -> Observation:
    return Observation(_u32(core, TASK0_ENTRY), state_machine_view(core.read(STATE_MACHINE, 12)))


Pattern = Callable[[int], frozenset[str]]
Settle = Optional[frozenset[str]]


def _drive(core: CoreSession, budget: int, arrived: Callable[[Observation], bool], pattern: Pattern,
           goal: str, settle: Settle) -> tuple[int, frozenset[str]]:
    """Advance one game frame at a time, holding `pattern(frame)` for each frame's pad fence, until
    `arrived(observation)` holds; then run one settle frame reading the settle pad, release, and park.

    The console commits each frame's pad one frame ahead (advance), so by the time it observes the
    arrival its arrival frame already reads pattern(arrival). That pad is returned as the settle
    pad; the product core, driven afterwards, is given it so its arrival frame reads the same pad
    and both park one frame past the transition in the same state. Returns (frames used, settle)."""
    lookahead = 1 if core.vblank_stepped else 0
    for frame in range(budget):
        seen = observe(core)
        if arrived(seen):
            if core.vblank_stepped:
                settle = core.held
            elif settle is None:
                raise ValueError(f"{core.name}: drive the console first; its arrival pad is the settle pad")
            core.hold(settle)
            advance(core, 1)
            core.hold(frozenset())
            return frame + 1, settle
        core.hold(pattern(frame + lookahead))
        advance(core, 1)
    seen = observe(core)
    raise Unreached(f"{core.name}: {goal} not reached within {budget} game frames "
                    f"(task0 entry 0x{seen.stage_entry:08X}, sm {seen.sm})")


def _tap(button: str, period: int, width: int = 6) -> Pattern:
    """Hold `button` for the first `width` frames of every `period` frames."""
    return lambda frame: frozenset({button}) if frame % period < width else frozenset()


def _release(frame: int) -> frozenset[str]:
    return frozenset()


def reach_game(core: CoreSession, budget: int, settle: Settle = None) -> tuple[int, frozenset[str]]:
    """Tap Cross for 6 of every 12 frames until task 0 runs the GAME stage (the product REPL's own
    `newgame` policy, expressed here so both cores receive the same input)."""
    return _drive(core, budget, lambda seen: seen.in_game_stage, _tap("cross", 12), "GAME stage", settle)


def reach_field(core: CoreSession, budget: int, settle: Settle = None) -> tuple[int, frozenset[str]]:
    """Skip the intro with Start (6 of every 24 frames) until the outer machine leaves it
    (sm[0x4a]!=0), then wait, without input, for the field to run. The settle pad belongs to the
    Start phase; the waiting phase always settles on a released pad."""
    used, settle = _drive(core, budget, lambda seen: seen.sm["4a"] != 0, _tap("start", 24), "intro skipped", settle)
    waited, _ = _drive(core, budget - used, lambda seen: seen.in_field, _release, "field", frozenset())
    return used + waited, settle


def reach_free_roam(core: CoreSession, budget: int, settle: Settle = None) -> tuple[int, frozenset[str]]:
    """Skip the field's opening cutscene with Start (6 of every 40 frames, the product's own
    auto-skip cadence) until the leaf machine hands the pad to the player."""
    return _drive(core, budget, lambda seen: seen.in_free_roam, _tap("start", 40), "free roam", settle)


def snapshot(core: CoreSession) -> dict[str, bytes]:
    return {declared.name: core.read(declared.address, declared.size) for declared in DECLARED}
