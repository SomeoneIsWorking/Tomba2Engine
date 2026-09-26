#!/usr/bin/env python3
"""Positive and negative tests for the shared Tomba! 2 navigation policy (tools/title_prompts.py).

The module is pure and BOTH drivers ask it, so a mistake in it is a mistake in the live play-through
AND in the two-core oracle comparison at once. These cases pin the two things that can be wrong
without anything failing loudly: the decode of one task-0 read, and the route's own shape.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "tools"))

# A plain import, not the importlib-by-path dance tests/test_run.py uses: @dataclass resolves its own
# module through sys.modules while the class body is executing, which a module that was never
# registered there cannot satisfy.
import title_prompts


def task0(stage: int, sm: tuple[int, int, int, int, int, int], cursor: int = 0) -> bytes:
    """A task-0 slot as the endpoint's `r 801FE000 6C` returns it: bytes at the offsets the C++ owns."""
    block = bytearray(title_prompts.TASK0_PROBE_BYTES)
    block[title_prompts.STAGE_ENTRY_OFFSET:title_prompts.STAGE_ENTRY_OFFSET + 4] = \
        stage.to_bytes(4, "little")
    for index, value in enumerate(sm):
        start = title_prompts.STATE_MACHINE_OFFSET + index * 2
        block[start:start + 2] = value.to_bytes(2, "little")
    block[title_prompts.CURSOR_OFFSET] = cursor
    return bytes(block)


def screen_of(stage: int, sm: tuple[int, int, int, int, int, int], cursor: int = 0):
    screen, decoded_cursor = title_prompts.screen_from_task0(task0(stage, sm, cursor))
    assert decoded_cursor == cursor
    return screen


class Task0BlockTest(unittest.TestCase):
    def test_one_read_decodes_the_stage_the_state_machine_and_the_cursor(self) -> None:
        screen, cursor = title_prompts.screen_from_task0(
            task0(title_prompts.STAGE_GAME, (2, 1, 2, 1, 0, 0), cursor=2))
        self.assertEqual(screen.stage_entry, title_prompts.STAGE_GAME)
        self.assertEqual(screen.sm, {"48": 2, "4a": 1, "4c": 2, "4e": 1, "50": 0, "52": 0})
        self.assertEqual(cursor, 2)

    def test_a_short_read_is_refused_rather_than_decoded(self) -> None:
        with self.assertRaises(ValueError):
            title_prompts.screen_from_task0(bytes(title_prompts.TASK0_PROBE_BYTES - 1))

    def test_free_roam_needs_the_field_and_the_leaf_machine(self) -> None:
        field = screen_of(title_prompts.STAGE_GAME, (2, 1, 2, 0, 0, 0))
        self.assertTrue(field.in_field)
        self.assertFalse(field.in_free_roam)          # the opening cutscene runs the leaf at 9
        self.assertTrue(screen_of(title_prompts.STAGE_GAME, (2, 1, 2, 9, 0, 0)).in_field)

    def test_the_GAME_stage_is_the_only_stage_that_counts_as_the_field(self) -> None:
        attract = screen_of(0x801062E4, (2, 1, 2, 1, 0, 0))   # the DEMO stage's own sm can look identical
        self.assertFalse(attract.in_game_stage)
        self.assertFalse(attract.in_field)
        self.assertFalse(attract.in_free_roam)

    def test_intro_is_left_when_the_outer_machine_moves_off_zero(self) -> None:
        self.assertFalse(screen_of(title_prompts.STAGE_GAME, (2, 0, 0, 0, 0, 0)).left_intro)
        for outer in (1, 2, 9):
            self.assertTrue(screen_of(title_prompts.STAGE_GAME, (2, outer, 0, 0, 0, 0)).left_intro)


class RouteTest(unittest.TestCase):
    def test_the_route_is_the_recorded_sequence(self) -> None:
        self.assertEqual(
            [(step.name, step.button, step.period, step.width) for step in title_prompts.GAMEPLAY_ROUTE],
            [("GAME stage", "cross", 12, 6),      # the product's own `newgame` cadence
             ("intro skipped", "start", 24, 6),
             ("field", None, 0, 0),               # hold nothing: the field arms on its own
             ("free roam", "start", 40, 6)])

    def test_each_legs_goal_is_the_next_legs_precondition(self) -> None:
        gameplay = screen_of(title_prompts.STAGE_GAME, (2, 1, 2, 1, 0, 0))
        for step in title_prompts.GAMEPLAY_ROUTE:
            self.assertTrue(step.reached(gameplay), step.name)

    def test_the_legs_are_walked_in_order_because_their_goals_overlap(self) -> None:
        # The front end runs the SAME state machine the field does, so a screen in the DEMO stage can
        # satisfy a later leg's goal. That is not a bug in the route: it is why both drivers walk the
        # legs in order, each goal only meaning something after the previous one has passed.
        boot_title = screen_of(0x801062E4, (2, 1, 0, 0, 0, 0))
        self.assertEqual([step.name for step in title_prompts.GAMEPLAY_ROUTE
                          if step.reached(boot_title)], ["intro skipped"])
        # ...and the boot state that satisfies NO leg at all, which is what a driver must keep driving.
        self.assertEqual([step.name for step in title_prompts.GAMEPLAY_ROUTE
                          if step.reached(screen_of(0x801062E4, (1, 0, 0, 0, 0, 0)))], [])
        # The GAME stage on its own satisfies exactly the first leg, which is the leg that gets it there.
        self.assertEqual([step.name for step in title_prompts.GAMEPLAY_ROUTE
                          if step.reached(screen_of(title_prompts.STAGE_GAME, (2, 0, 0, 0, 0, 0)))],
                         ["GAME stage"])

    def test_idle_frames_is_the_gap_a_transport_waits_between_presses(self) -> None:
        self.assertEqual(title_prompts.GAMEPLAY_ROUTE[0].idle_frames, 6)   # 12-frame period, 6 pressed
        self.assertEqual(title_prompts.GAMEPLAY_ROUTE[3].idle_frames, 34)
        self.assertEqual(title_prompts.GAMEPLAY_ROUTE[2].idle_frames, 0)   # the leg presses nothing


class AddressOwnerTest(unittest.TestCase):
    def test_every_address_this_tool_reads_is_still_declared_by_its_source(self) -> None:
        matched, scanned, disagreements = title_prompts.verify_address_owners(REPO)
        self.assertEqual(disagreements, [])
        self.assertEqual(matched, scanned)
        self.assertGreater(scanned, 0)

    def test_a_moved_address_is_reported_rather_than_used(self) -> None:
        matched, scanned, disagreements = title_prompts.verify_address_owners(REPO / "tools")
        self.assertEqual(matched, 0)
        self.assertEqual(scanned, len(title_prompts.ADDRESS_OWNERS))
        self.assertEqual(len(disagreements), scanned)


if __name__ == "__main__":
    unittest.main()
