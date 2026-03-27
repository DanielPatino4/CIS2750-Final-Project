"""Student Python tests for A2 wrappers and integration helpers."""

import os
import unittest

from treasure_runner.bindings import Direction
from treasure_runner.bindings import Status
from treasure_runner.models.exceptions import NoPortalError
from treasure_runner.models.exceptions import StatusError
from treasure_runner.models.exceptions import status_to_exception
from treasure_runner.models.exceptions import status_to_status_exception
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.player import Player
from run_integration import get_state, state_to_str


def _repo_root() -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def _pick_config() -> str:
    root = _repo_root()
    candidates = [
        os.path.join(root, "assets", "treasure_runner.ini"),
        os.path.join(root, "assets", "starter.ini"),
        os.path.join(root, "A2_instructions", "example_integration_run.ini"),
    ]
    for path in candidates:
        if os.path.exists(path):
            return path
    raise FileNotFoundError("No suitable config file found for Python tests")


class TestStudentPython(unittest.TestCase):
    """Basic behavioral tests to exercise wrappers and helpers."""

    def setUp(self):
        self.engine = GameEngine(_pick_config())

    def tearDown(self):
        self.engine.destroy()

    def test_player_wrapper_exists(self):
        self.assertIsInstance(self.engine.player, Player)

    def test_state_string_format(self):
        state = get_state(self.engine)
        value = state_to_str(state)
        self.assertIn("room=", value)
        self.assertIn("|x=", value)
        self.assertIn("|y=", value)
        self.assertIn("|collected=", value)

    def test_engine_metadata(self):
        room_count = self.engine.get_room_count()
        width, height = self.engine.get_room_dimensions()
        room_ids = self.engine.get_room_ids()

        self.assertGreaterEqual(room_count, 1)
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)
        self.assertEqual(len(room_ids), room_count)

    def test_render_current_room(self):
        rendered = self.engine.render_current_room()
        self.assertIsInstance(rendered, str)
        self.assertIn("\n", rendered)

    def test_player_methods(self):
        room_id = self.engine.player.get_room()
        pos_x, pos_y = self.engine.player.get_position()

        self.assertIsInstance(room_id, int)
        self.assertIsInstance(pos_x, int)
        self.assertIsInstance(pos_y, int)
        self.assertGreaterEqual(self.engine.player.get_collected_count(), 0)
        self.assertIsInstance(self.engine.player.get_collected_treasures(), list)
        self.assertIsInstance(self.engine.player.has_collected_treasure(999999), bool)

    def test_reset_roundtrip(self):
        start_state = get_state(self.engine)
        for direction in [Direction.NORTH, Direction.SOUTH, Direction.EAST, Direction.WEST]:
            try:
                self.engine.move_player(direction)
            except Exception:
                pass
        self.engine.reset()
        self.assertEqual(get_state(self.engine), start_state)

    def test_use_portal_callable(self):
        self.assertTrue(callable(self.engine.use_portal))
        try:
            self.engine.use_portal()
        except NoPortalError:
            pass

    def test_total_treasure_count(self):
        total = self.engine.get_total_treasure_count()
        self.assertIsInstance(total, int)
        self.assertGreater(total, 0)
        self.assertFalse(self.engine.all_treasure_collected())

    def test_status_to_exception_mappings(self):
        exc = status_to_exception(Status.ROOM_NO_PORTAL, "no portal")
        self.assertIsInstance(exc, NoPortalError)

        exc = status_to_exception(999999, "unknown")
        self.assertIsInstance(exc, Exception)

    def test_status_to_status_exception_mappings(self):
        exc = status_to_status_exception(Status.WL_ERR_CONFIG, "config")
        self.assertIsInstance(exc, Exception)

        exc = status_to_status_exception(999999, "unknown")
        self.assertIsInstance(exc, StatusError)


if __name__ == "__main__":
    unittest.main()
