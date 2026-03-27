"""Coverage-focused tests for run_integration helpers and flow."""

from __future__ import annotations

import io
import os
import tempfile
import unittest
from argparse import Namespace
from unittest.mock import patch

import run_integration as ri
from treasure_runner.bindings import Direction
from treasure_runner.models.exceptions import GameError, ImpassableError


class _FakePlayer:
    def __init__(self, room: int = 0, x: int = 1, y: int = 1, collected: int = 0):
        self.room = room
        self.x = x
        self.y = y
        self.collected = collected

    def get_room(self):
        return self.room

    def get_position(self):
        return (self.x, self.y)

    def get_collected_count(self):
        return self.collected


class _DirectionEngine:
    def __init__(self, outcomes_by_direction: dict[Direction, str]):
        self.player = _FakePlayer()
        self._outcomes = outcomes_by_direction
        self.reset_calls = 0

    def move_player(self, direction: Direction):
        outcome = self._outcomes.get(direction, "impassable")
        if outcome == "impassable":
            raise ImpassableError("blocked")
        if outcome == "error":
            raise GameError("err")
        if outcome == "move_same_room":
            self.player.x += 1
            return
        if outcome == "move_room_change":
            self.player.room += 1
            return

    def reset(self):
        self.reset_calls += 1
        self.player = _FakePlayer()


class _ScriptedEngine:
    def __init__(self, states_or_errors: list[tuple[int, int, int, int] | str]):
        self.player = _FakePlayer()
        self._script = states_or_errors[:]

    def move_player(self, _direction: Direction):
        if not self._script:
            raise ImpassableError("blocked")
        event = self._script.pop(0)
        if event == "impassable":
            raise ImpassableError("blocked")
        if event == "error":
            raise GameError("error")
        room, x, y, collected = event
        self.player.room = room
        self.player.x = x
        self.player.y = y
        self.player.collected = collected


class _MainEngine:
    def __init__(self, _config_path: str, fail_entry: bool = False):
        self.player = _FakePlayer(room=0, x=1, y=1, collected=0)
        self._fail_entry = fail_entry
        self.destroy_called = False

    def get_room_count(self):
        return 3

    def get_room_dimensions(self):
        return (10, 10)

    def move_player(self, _direction: Direction):
        if self._fail_entry:
            raise GameError("entry fail")
        self.player.x += 1

    def reset(self):
        self.player = _FakePlayer(room=0, x=1, y=1, collected=0)

    def destroy(self):
        self.destroy_called = True


class TestRunIntegration(unittest.TestCase):
    def test_helpers(self):
        player_engine = _DirectionEngine({})
        state = ri.get_state(player_engine)
        self.assertEqual(state, (0, 1, 1, 0))
        self.assertEqual(ri.state_to_str(state), "room=0|x=1|y=1|collected=0")

        handle = io.StringIO()
        ri.write_line(handle, "EVENT", a=1, b="x")
        self.assertEqual(handle.getvalue(), "EVENT|a=1|b=x\n")

    def test_find_entry_direction_success(self):
        engine = _DirectionEngine(
            {
                Direction.SOUTH: "impassable",
                Direction.WEST: "move_same_room",
                Direction.NORTH: "error",
                Direction.EAST: "move_room_change",
            }
        )
        found = ri.find_entry_direction(engine)
        self.assertEqual(found, Direction.WEST)
        self.assertGreaterEqual(engine.reset_calls, 1)

    def test_find_entry_direction_failure(self):
        engine = _DirectionEngine(
            {
                Direction.SOUTH: "impassable",
                Direction.WEST: "error",
                Direction.NORTH: "move_room_change",
                Direction.EAST: "impassable",
            }
        )
        with self.assertRaises(RuntimeError):
            ri.find_entry_direction(engine)

    def test_run_sweep_blocked_and_cycle_paths(self):
        blocked_engine = _ScriptedEngine(["impassable"])
        blocked_handle = io.StringIO()
        step = ri.run_sweep(blocked_engine, blocked_handle, "SWEEP_A", Direction.SOUTH, 0)
        self.assertGreaterEqual(step, 1)
        self.assertIn("SWEEP_END|phase=SWEEP_A|reason=BLOCKED", blocked_handle.getvalue())

        cycle_engine = _ScriptedEngine([(0, 2, 1, 0), (0, 1, 1, 0)])
        cycle_handle = io.StringIO()
        step = ri.run_sweep(cycle_engine, cycle_handle, "SWEEP_B", Direction.EAST, 0)
        self.assertGreaterEqual(step, 2)
        self.assertIn("SWEEP_END|phase=SWEEP_B|reason=CYCLE_DETECTED", cycle_handle.getvalue())

    def test_parse_args(self):
        with patch(
            "sys.argv",
            ["run_integration.py", "--config", "assets/starter.ini", "--log", "x.log"],
        ):
            args = ri.parse_args()
            self.assertEqual(args.config, "assets/starter.ini")
            self.assertEqual(args.log, "x.log")

    def test_main_success(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            log_path = os.path.join(temp_dir, "integration.log")
            fake_args = Namespace(config="assets/starter.ini", log=log_path)

            with patch.object(ri, "parse_args", return_value=fake_args), patch.object(
                ri, "GameEngine", side_effect=lambda cfg: _MainEngine(cfg, fail_entry=False)
            ), patch.object(ri, "find_entry_direction", return_value=Direction.EAST), patch.object(
                ri, "run_sweep", side_effect=lambda _e, _h, _p, _d, step: step + 1
            ):
                result = ri.main()

            self.assertEqual(result, 0)
            with open(log_path, "r", encoding="utf-8") as handle:
                log_text = handle.read()
            self.assertIn("RUN_START", log_text)
            self.assertIn("RUN_END", log_text)

    def test_main_entry_error_path(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            log_path = os.path.join(temp_dir, "integration_error.log")
            fake_args = Namespace(config="assets/starter.ini", log=log_path)

            with patch.object(ri, "parse_args", return_value=fake_args), patch.object(
                ri, "GameEngine", side_effect=lambda cfg: _MainEngine(cfg, fail_entry=True)
            ), patch.object(ri, "find_entry_direction", return_value=Direction.EAST):
                result = ri.main()

            self.assertEqual(result, 0)
            with open(log_path, "r", encoding="utf-8") as handle:
                log_text = handle.read()
            self.assertIn("TERMINATED: Initial Move Error", log_text)
            self.assertIn("RUN_END", log_text)


if __name__ == "__main__":
    unittest.main()
