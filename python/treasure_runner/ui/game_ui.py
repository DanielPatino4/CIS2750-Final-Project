"""Curses-based user interface for Treasure Runner."""

from __future__ import annotations

import curses
from typing import Any

from ..bindings import Direction
from ..models.exceptions import GameError, ImpassableError, NoPortalError


class GameUI:
    """Render the game with curses and translate keypresses into actions."""

    def __init__(self, engine, title: str = "Treasure Run", email: str = "dpatino@uoguelph.ca"):
        self._engine = engine
        self._title = title
        self._email = email
        self._message = "Welcome to Treasure Run."
        self._player_name = "Player"
        self._visited_rooms: set[int] = set()
        self._quit_reason = "Quit requested"
        self._colors_enabled = False
        self._graph_room_ids: list[int] = []
        self._graph_adjacency: list[list[bool]] = []

    def run(self, profile: dict[str, Any]) -> dict[str, Any]:
        """Launch the curses UI and return a session summary."""
        return curses.wrapper(self._curses_main, dict(profile))

    def _curses_main(self, stdscr, profile: dict[str, Any]) -> dict[str, Any]:
        curses.cbreak()
        curses.noecho()
        stdscr.keypad(True)
        stdscr.nodelay(False)
        try:
            curses.curs_set(0)
        except curses.error:
            pass

        self._player_name = self._show_startup_screen(stdscr, profile)
        self._visited_rooms = {self._engine.get_current_room_id()}
        self._message = f"Welcome, {self._player_name}."
        self._init_colors()

        try:
            self._graph_room_ids, self._graph_adjacency = self._engine.get_adjacency_matrix()
        except GameError:
            self._graph_room_ids = []
            self._graph_adjacency = []

        while True:
            self._draw_game_screen(stdscr)
            key = stdscr.getch()

            if key in (ord("q"), ord("Q")):
                self._quit_reason = "Quit requested"
                break

            if key in (ord("r"), ord("R")):
                self._engine.reset()
                self._visited_rooms = {self._engine.get_current_room_id()}
                self._message = "Game reset to the starting state."
                continue

            if key == ord(">"):
                self._handle_portal_use()
                continue

            direction = self._key_to_direction(key)
            if direction is None:
                self._message = "Use arrows/WASD to move, r to reset, q to quit."
                continue

            if self._handle_move(direction):
                self._quit_reason = "Victory: collected all treasure"
                break

        summary = self._build_session_summary()
        self._show_end_screen(stdscr, profile, summary)
        return summary

    def _handle_portal_use(self) -> None:
        before_room = self._engine.get_current_room_id()

        try:
            self._engine.use_portal()
        except ImpassableError:
            self._message = "That portal is locked. Activate its switch first."
            return
        except NoPortalError:
            self._message = "There is no portal here to use."
            return
        except GameError as exc:
            self._message = str(exc) or "The portal could not be used."
            return

        after_room = self._engine.get_current_room_id()
        self._visited_rooms.add(after_room)
        if after_room != before_room:
            self._message = f"Traversed portal to room {after_room}."
        else:
            self._message = "Used the portal."

    def _handle_move(self, direction: Direction) -> bool:
        before = self._engine.get_status_snapshot()
        before_treasures = self._engine.get_collected_treasures()

        try:
            self._engine.move_player(direction)
        except ImpassableError:
            self._message = "You cannot go that way."
            return False
        except GameError as exc:
            self._message = str(exc) or "That move failed."
            return False

        after = self._engine.get_status_snapshot()
        self._visited_rooms.add(after["room_id"])
        return self._apply_move_result(before, before_treasures, after, direction)

    def _apply_move_result(
        self,
        before: dict[str, Any],
        before_treasures: list[dict[str, Any]],
        after: dict[str, Any],
        direction: Direction,
    ) -> bool:
        if self._is_victory(after):
            self._message = "Victory! You collected all treasure."
            return True

        if after["collected_count"] > before["collected_count"]:
            self._update_treasure_pickup_message(before_treasures)
            return False

        if after["room_id"] != before["room_id"]:
            self._message = f"Entered room {after['room_id']}."
            return False

        self._message = f"Moved {direction.name.lower()}."
        return False

    def _update_treasure_pickup_message(self, before_treasures: list[dict[str, Any]]) -> None:
        new_treasures = self._engine.get_collected_treasures()
        if len(new_treasures) > len(before_treasures):
            latest = new_treasures[-1].get("name") or "a treasure"
            self._message = f"You picked up {latest}."
            return

        self._message = "You picked up a treasure."

    def _is_victory(self, snapshot: dict[str, Any]) -> bool:
        total_treasure_count = int(snapshot.get("total_treasure_count", 0))
        collected_count = int(snapshot.get("collected_count", 0))
        return 0 < total_treasure_count <= collected_count

    def _draw_game_screen(self, stdscr) -> None:
        snapshot = self._engine.get_status_snapshot()
        room_lines = self._room_lines()
        self._ensure_terminal_size(stdscr, room_lines)

        stdscr.erase()
        self._safe_addstr(stdscr, 0, 0, f"Message: {self._message}", curses.A_BOLD)
        self._safe_addstr(stdscr, 1, 0, f"Room {snapshot['room_id']}")

        self._draw_room_lines(stdscr, room_lines)
        self._draw_minimap(stdscr, snapshot, room_lines)
        self._draw_footer(stdscr, snapshot, room_lines)
        stdscr.refresh()

    def _init_colors(self) -> None:
        if not curses.has_colors():
            self._colors_enabled = False
            return

        curses.start_color()
        try:
            curses.use_default_colors()
        except curses.error:
            pass

        curses.init_pair(1, curses.COLOR_GREEN, -1)   # current room
        curses.init_pair(2, curses.COLOR_CYAN, -1)    # visited room
        curses.init_pair(3, curses.COLOR_WHITE, -1)   # unvisited room
        curses.init_pair(4, curses.COLOR_YELLOW, -1)  # header/legend
        curses.init_pair(5, curses.COLOR_BLUE, -1)    # wall
        curses.init_pair(6, curses.COLOR_WHITE, -1)   # floor
        curses.init_pair(7, curses.COLOR_GREEN, -1)   # player
        curses.init_pair(8, curses.COLOR_YELLOW, -1)  # treasure
        curses.init_pair(9, curses.COLOR_MAGENTA, -1) # portal
        curses.init_pair(10, curses.COLOR_CYAN, -1)   # pushable
        self._colors_enabled = True

    def _room_attr(self, room_id: int, current_room_id: int) -> int:
        if not self._colors_enabled:
            return curses.A_BOLD if room_id == current_room_id else 0

        if room_id == current_room_id:
            return curses.color_pair(1) | curses.A_BOLD
        if room_id in self._visited_rooms:
            return curses.color_pair(2)
        return curses.color_pair(3)

    def _draw_minimap(self, stdscr, snapshot: dict[str, Any], room_lines: list[str]) -> None:
        room_width = max((len(line) for line in room_lines), default=0)
        panel_col = room_width + 6
        panel_row = 3

        title_attr = (curses.color_pair(4) | curses.A_BOLD) if self._colors_enabled else curses.A_BOLD
        self._safe_addstr(stdscr, panel_row, panel_col, "Mini-map (room graph)", title_attr)

        if not self._graph_room_ids or not self._graph_adjacency:
            self._safe_addstr(stdscr, panel_row + 1, panel_col, "Graph unavailable")
            return

        ids = self._graph_room_ids
        max_rooms = min(len(ids), 10)
        ids = ids[:max_rooms]

        header = "     " + " ".join(f"{room_id:>2}" for room_id in ids)
        self._safe_addstr(stdscr, panel_row + 1, panel_col, header)

        current_room_id = int(snapshot["room_id"])
        for row_index, from_room_id in enumerate(ids):
            marker = "@" if from_room_id == current_room_id else ("v" if from_room_id in self._visited_rooms else ".")
            row_label = f"{marker}{from_room_id:>3} "
            row = ["1" if self._graph_adjacency[row_index][col_index] else "." for col_index in range(max_rooms)]
            row_text = " ".join(f"{cell:>2}" for cell in row)

            draw_row = panel_row + 2 + row_index
            self._safe_addstr(stdscr, draw_row, panel_col, row_label, self._room_attr(from_room_id, current_room_id))
            self._safe_addstr(stdscr, draw_row, panel_col + len(row_label), row_text)

        legend_row = panel_row + 3 + max_rooms
        self._safe_addstr(stdscr, legend_row, panel_col, "@ current  v visited  . unvisited")
        self._safe_addstr(stdscr, legend_row + 1, panel_col, "Matrix: row -> column unique room link")
        self._safe_addstr(stdscr, legend_row + 2, panel_col, "(multiple portals to same room count as one link)")

    def _draw_room_lines(self, stdscr, room_lines: list[str]) -> None:
        for row_index, line in enumerate(room_lines, start=3):
            if not self._colors_enabled:
                self._safe_addstr(stdscr, row_index, 2, line)
                continue

            for col_offset, ch in enumerate(line):
                attr = self._tile_attr(ch)
                self._safe_addstr(stdscr, row_index, 2 + col_offset, ch, attr)

    def _tile_attr(self, tile: str) -> int:
        if not self._colors_enabled or not tile:
            return 0

        ch = tile[0]
        if ch == "#":
            return curses.color_pair(5)
        if ch == ".":
            return curses.color_pair(6)
        if ch == "@":
            return curses.color_pair(7) | curses.A_BOLD
        if ch == "$":
            return curses.color_pair(8) | curses.A_BOLD
        if ch == "X":
            return curses.color_pair(9) | curses.A_BOLD
        if ch == "O":
            return curses.color_pair(10) | curses.A_BOLD

        return curses.color_pair(6)

    def _draw_footer(self, stdscr, snapshot: dict[str, Any], room_lines: list[str]) -> None:
        controls_row = 4 + len(room_lines)
        status_row = controls_row + 1
        title_row = status_row + 2
        email_row = title_row + 1

        self._safe_addstr(stdscr, controls_row, 0, self._controls_text())
        self._safe_addstr(stdscr, status_row, 0, self._status_text(snapshot))
        self._safe_addstr(stdscr, title_row, 0, self._title, curses.A_BOLD)
        self._safe_addstr(stdscr, email_row, 0, self._email)

    def _controls_text(self) -> str:
        return "Controls: arrows/WASD move | > portal | r reset | q quit"

    def _status_text(self, snapshot: dict[str, Any]) -> str:
        pos_x, pos_y = snapshot["position"]
        collected_count = int(snapshot["collected_count"])
        total_treasure_count = int(snapshot.get("total_treasure_count", 0))
        return (
            f"Player: {self._player_name} | Gold: {collected_count}/{total_treasure_count} "
            f"| Rooms visited: {len(self._visited_rooms)}/{snapshot['room_count']} "
            f"| Pos: ({pos_x}, {pos_y})"
        )

    def _show_startup_screen(self, stdscr, profile: dict[str, Any]) -> str:
        existing_name = str(profile.get("player_name") or "").strip()
        summary_lines = self._profile_summary_lines(profile)
        if existing_name:
            summary_lines.append("")
            summary_lines.append("Press any key to start.")
            self._show_splash(stdscr, "Treasure Run", summary_lines)
            stdscr.getch()
            return existing_name

        lines = summary_lines + ["", "No player profile found yet.", "Press any key to enter your name."]
        self._show_splash(stdscr, "Treasure Run", lines)
        stdscr.getch()
        return self._prompt_for_name(stdscr)

    def _show_end_screen(self, stdscr, profile: dict[str, Any], summary: dict[str, Any]) -> None:
        heading = "Victory!" if str(summary.get("quit_reason", "")).startswith("Victory") else "Run Complete"
        lines = self._profile_summary_lines(profile) + [
            "",
            f"Quit reason: {summary['quit_reason']}",
            (
                "Treasure collected this run: "
                f"{summary['collected_count']}/{summary.get('total_treasure_count', summary['collected_count'])}"
            ),
            f"Rooms visited this run: {summary['rooms_visited']}",
            "",
            "Press any key to continue.",
        ]
        self._show_splash(stdscr, heading, lines)
        stdscr.getch()

    def _show_splash(self, stdscr, heading: str, lines: list[str]) -> None:
        stdscr.erase()
        max_y, max_x = stdscr.getmaxyx()
        required_height = len(lines) + 4
        if max_y < required_height or max_x < 30:
            raise RuntimeError("Terminal too small for the splash screen.")

        start_row = max((max_y - required_height) // 2, 0)
        self._safe_addstr(stdscr, start_row, 0, heading, curses.A_BOLD)
        for offset, line in enumerate(lines, start=2):
            self._safe_addstr(stdscr, start_row + offset, 0, line)
        stdscr.refresh()

    def _prompt_for_name(self, stdscr) -> str:
        prompt = "Enter player name: "
        while True:
            stdscr.erase()
            self._safe_addstr(stdscr, 0, 0, "Treasure Run", curses.A_BOLD)
            self._safe_addstr(stdscr, 2, 0, prompt)
            stdscr.refresh()

            curses.echo()
            try:
                try:
                    curses.curs_set(1)
                except curses.error:
                    pass
                name_bytes = stdscr.getstr(2, len(prompt), 40)
            finally:
                curses.noecho()
                try:
                    curses.curs_set(0)
                except curses.error:
                    pass

            name = name_bytes.decode("utf-8", errors="ignore").strip()
            if name:
                return name

            self._safe_addstr(stdscr, 4, 0, "Name cannot be empty. Press any key to try again.")
            stdscr.refresh()
            stdscr.getch()

    def _profile_summary_lines(self, profile: dict[str, Any]) -> list[str]:
        player_name = str(profile.get("player_name") or "<new player>")
        return [
            f"Player: {player_name}",
            f"Games played: {int(profile.get('games_played', 0))}",
            (
                "Max treasure collected: "
                f"{int(profile.get('max_treasure_collected', 0))}"
            ),
            (
                "Most rooms completed: "
                f"{int(profile.get('most_rooms_world_completed', 0))}"
            ),
            f"Last played: {profile.get('timestamp_last_played', 'Never')}",
        ]

    def _build_session_summary(self) -> dict[str, Any]:
        snapshot = self._engine.get_status_snapshot()
        return {
            "player_name": self._player_name,
            "collected_count": snapshot["collected_count"],
            "total_treasure_count": snapshot.get("total_treasure_count", snapshot["collected_count"]),
            "rooms_visited": len(self._visited_rooms),
            "quit_reason": self._quit_reason,
        }

    def _ensure_terminal_size(self, stdscr, room_lines: list[str]) -> None:
        max_y, max_x = stdscr.getmaxyx()
        room_width = max((len(line) for line in room_lines), default=0)
        graph_rows = max(7, min(len(self._graph_room_ids), 10) + 6)
        min_height = max(len(room_lines) + 9, 3 + graph_rows)
        min_width = max(room_width + 42, 86)
        if max_y < min_height or max_x < min_width:
            raise RuntimeError(
                f"Terminal too small. Need at least {min_width}x{min_height}."
            )

    def _room_lines(self) -> list[str]:
        rendered = self._engine.render_current_room().rstrip("\n")
        return rendered.splitlines() if rendered else []

    def _key_to_direction(self, key: int) -> Direction | None:
        mapping = {
            curses.KEY_UP: Direction.NORTH,
            curses.KEY_DOWN: Direction.SOUTH,
            curses.KEY_LEFT: Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
            ord("w"): Direction.NORTH,
            ord("W"): Direction.NORTH,
            ord("s"): Direction.SOUTH,
            ord("S"): Direction.SOUTH,
            ord("a"): Direction.WEST,
            ord("A"): Direction.WEST,
            ord("d"): Direction.EAST,
            ord("D"): Direction.EAST,
        }
        return mapping.get(key)

    def _safe_addstr(self, stdscr, row: int, col: int, text: str, attr: int = 0) -> None:
        max_y, max_x = stdscr.getmaxyx()
        if row < 0 or row >= max_y or col >= max_x:
            return
        clipped = text[: max_x - col - 1]
        if not clipped:
            return
        try:
            stdscr.addstr(row, col, clipped, attr)
        except curses.error:
            pass
