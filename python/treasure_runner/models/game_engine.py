"""High-level GameEngine wrapper around C engine APIs."""

import ctypes

from ..bindings import GameEngine as GameEnginePtr
from ..bindings import Status, lib
from .exceptions import status_to_exception
from .player import Player


class GameEngine:
    """Owns a C GameEngine pointer and exposes Pythonic methods."""

    def __init__(self, config_path: str):
        self._eng = GameEnginePtr()
        self._player = None
        self._destroyed = False

        status = lib.game_engine_create(config_path.encode("utf-8"), ctypes.byref(self._eng))
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_create failed")

        player_ptr = lib.game_engine_get_player(self._eng)
        if not bool(player_ptr):
            lib.game_engine_destroy(self._eng)
            raise RuntimeError("game_engine_get_player failed")
        self._player = Player(player_ptr)

    @property
    def player(self) -> Player:
        return self._player

    def _require_engine(self):
        if self._destroyed or not bool(self._eng):
            raise RuntimeError("GameEngine has been destroyed")
        return self._eng

    def destroy(self) -> None:
        if self._destroyed:
            return
        if bool(self._eng):
            lib.game_engine_destroy(self._eng)
        self._eng = GameEnginePtr()
        self._player = None
        self._destroyed = True

    def move_player(self, direction) -> None:
        eng = self._require_engine()
        status = lib.game_engine_move_player(eng, int(direction))
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_move_player failed")

    def render_current_room(self) -> str:
        eng = self._require_engine()
        c_str = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(eng, ctypes.byref(c_str))
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_render_current_room failed")

        if not bool(c_str):
            return ""

        try:
            return (c_str.value or b"").decode("utf-8")
        finally:
            lib.game_engine_free_string(ctypes.cast(c_str, ctypes.c_void_p))

    def get_room_count(self) -> int:
        eng = self._require_engine()
        count_out = ctypes.c_int()
        status = lib.game_engine_get_room_count(eng, ctypes.byref(count_out))
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_get_room_count failed")
        return int(count_out.value)

    def get_room_dimensions(self) -> tuple[int, int]:
        eng = self._require_engine()
        width_out = ctypes.c_int()
        height_out = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(
            eng, ctypes.byref(width_out), ctypes.byref(height_out)
        )
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_get_room_dimensions failed")
        return (int(width_out.value), int(height_out.value))

    def get_room_ids(self) -> list[int]:
        eng = self._require_engine()
        ids_out = ctypes.POINTER(ctypes.c_int)()
        count_out = ctypes.c_int()

        status = lib.game_engine_get_room_ids(eng, ctypes.byref(ids_out), ctypes.byref(count_out))
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_get_room_ids failed")

        count = int(count_out.value)
        if count <= 0 or not bool(ids_out):
            return []

        try:
            return [int(ids_out[i]) for i in range(count)]
        finally:
            lib.game_engine_free_string(ctypes.cast(ids_out, ctypes.c_void_p))

    def get_current_room_id(self) -> int:
        return self.player.get_room()

    def get_player_position(self) -> tuple[int, int]:
        return self.player.get_position()

    def get_collected_count(self) -> int:
        return self.player.get_collected_count()

    def get_collected_treasures(self) -> list[dict]:
        return self.player.get_collected_treasures()

    def get_status_snapshot(self) -> dict:
        pos_x, pos_y = self.get_player_position()
        return {
            "room_id": self.get_current_room_id(),
            "position": (pos_x, pos_y),
            "collected_count": self.get_collected_count(),
            "room_count": self.get_room_count(),
        }

    def reset(self) -> None:
        eng = self._require_engine()
        status = lib.game_engine_reset(eng)
        if status != Status.OK:
            raise status_to_exception(status, "game_engine_reset failed")
