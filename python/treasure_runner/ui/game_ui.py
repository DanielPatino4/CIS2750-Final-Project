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

            self._handle_move(direction)

        summary = self._build_session_summary()
        self._show_end_screen(stdscr, profile, summary)
        return summary

    def _handle_portal_use(self) -> None:
        before_room = self._engine.get_current_room_id()

        try:
            self._engine.use_portal()
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

    def _handle_move(self, direction: Direction) -> None:
        before = self._engine.get_status_snapshot()
        before_treasures = self._engine.get_collected_treasures()

        try:
            self._engine.move_player(direction)
        except ImpassableError:
            self._message = "You cannot go that way."
            return
        except GameError as exc:
            self._message = str(exc) or "That move failed."
            return

        after = self._engine.get_status_snapshot()
        self._visited_rooms.add(after["room_id"])

        if after["collected_count"] > before["collected_count"]:
            new_treasures = self._engine.get_collected_treasures()
            if len(new_treasures) > len(before_treasures):
                latest = new_treasures[-1].get("name") or "a treasure"
                self._message = f"You picked up {latest}."
            else:
                self._message = "You picked up a treasure."
            return

        if after["room_id"] != before["room_id"]:
            self._message = f"Entered room {after['room_id']}."
            return

        self._message = f"Moved {direction.name.lower()}."

    def _draw_game_screen(self, stdscr) -> None:
        snapshot = self._engine.get_status_snapshot()
        room_lines = self._room_lines()
        self._ensure_terminal_size(stdscr, room_lines)

        stdscr.erase()
        self._safe_addstr(stdscr, 0, 0, f"Message: {self._message}", curses.A_BOLD)
        self._safe_addstr(stdscr, 1, 0, f"Room {snapshot['room_id']}")

        self._draw_room_lines(stdscr, room_lines)
        self._draw_footer(stdscr, snapshot, room_lines)
        stdscr.refresh()

    def _draw_room_lines(self, stdscr, room_lines: list[str]) -> None:
        for index, line in enumerate(room_lines, start=3):
            self._safe_addstr(stdscr, index, 2, line)

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
        return (
            f"Player: {self._player_name} | Gold: {snapshot['collected_count']} "
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
        lines = self._profile_summary_lines(profile) + [
            "",
            f"Quit reason: {summary['quit_reason']}",
            f"Treasure collected this run: {summary['collected_count']}",
            f"Rooms visited this run: {summary['rooms_visited']}",
            "",
            "Press any key to continue.",
        ]
        self._show_splash(stdscr, "Run Complete", lines)
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
            "rooms_visited": len(self._visited_rooms),
            "quit_reason": self._quit_reason,
        }

    def _ensure_terminal_size(self, stdscr, room_lines: list[str]) -> None:
        max_y, max_x = stdscr.getmaxyx()
        room_width = max((len(line) for line in room_lines), default=0)
        min_height = len(room_lines) + 9
        min_width = max(room_width + 4, 64)
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
