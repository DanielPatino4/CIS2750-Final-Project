"""High-level Player wrapper around C player APIs."""

import ctypes

from ..bindings import Status, Treasure, lib
from .exceptions import status_to_status_exception


class Player:
    """Wrap a non-owning C Player pointer."""

    def __init__(self, ptr):
        self._ptr = ptr

    def get_room(self) -> int:
        return int(lib.player_get_room(self._ptr))

    def get_position(self) -> tuple[int, int]:
        x_out = ctypes.c_int()
        y_out = ctypes.c_int()
        status = lib.player_get_position(self._ptr, ctypes.byref(x_out), ctypes.byref(y_out))
        if status != Status.OK:
            raise status_to_status_exception(status, "player_get_position failed")
        return (int(x_out.value), int(y_out.value))

    def get_collected_count(self) -> int:
        return int(lib.player_get_collected_count(self._ptr))

    def has_collected_treasure(self, treasure_id: int) -> bool:
        return bool(lib.player_has_collected_treasure(self._ptr, int(treasure_id)))

    def get_collected_treasures(self) -> list[dict]:
        count_out = ctypes.c_int()
        treasures = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count_out))
        count = int(count_out.value)
        if count <= 0:
            return []

        result = []
        for index in range(count):
            treasure_ptr = treasures[index]
            if not bool(treasure_ptr):
                continue
            treasure: Treasure = treasure_ptr.contents
            name_value = treasure.name.decode("utf-8") if treasure.name is not None else None
            result.append(
                {
                    "id": int(treasure.id),
                    "name": name_value,
                    "starting_room_id": int(treasure.starting_room_id),
                    "initial_x": int(treasure.initial_x),
                    "initial_y": int(treasure.initial_y),
                    "x": int(treasure.x),
                    "y": int(treasure.y),
                    "collected": bool(treasure.collected),
                }
            )
        return result
