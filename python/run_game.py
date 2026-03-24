#!/usr/bin/env python3
"""Launch the A3 Treasure Runner curses interface."""

from __future__ import annotations

import argparse
import json
import os
import sys
from copy import deepcopy
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

THIS_DIR = Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui import GameUI

DEFAULT_PROFILE = {
    "player_name": "",
    "games_played": 0,
    "max_treasure_collected": 0,
    "most_rooms_world_completed": 0,
    "timestamp_last_played": "Never",
}


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments for the curses runner."""
    parser = argparse.ArgumentParser(description="Treasure Runner game launcher")
    parser.add_argument("--config", required=True, help="Path to the world config .ini file")
    parser.add_argument("--profile", required=True, help="Path to the player profile JSON file")
    return parser.parse_args()


def load_profile(profile_path: str) -> dict[str, Any]:
    """Load an existing profile or return a default skeleton."""
    profile = deepcopy(DEFAULT_PROFILE)
    if not os.path.exists(profile_path):
        return profile

    try:
        with open(profile_path, "r", encoding="utf-8") as handle:
            loaded = json.load(handle)
    except (OSError, json.JSONDecodeError):
        return profile

    if not isinstance(loaded, dict):
        return profile

    for key in profile:
        if key in loaded:
            profile[key] = loaded[key]

    return profile


def save_profile(profile_path: str, profile: dict[str, Any]) -> None:
    """Persist the profile JSON to disk."""
    parent = os.path.dirname(profile_path)
    if parent:
        os.makedirs(parent, exist_ok=True)

    with open(profile_path, "w", encoding="utf-8") as handle:
        json.dump(profile, handle, indent=2)
        handle.write("\n")


def finalize_profile(profile: dict[str, Any], session: dict[str, Any]) -> dict[str, Any]:
    """Merge this run's results into the persisted profile data."""
    updated = deepcopy(profile)
    player_name = str(session.get("player_name") or "").strip()
    if player_name:
        updated["player_name"] = player_name

    updated["games_played"] = int(updated.get("games_played", 0)) + 1
    updated["max_treasure_collected"] = max(
        int(updated.get("max_treasure_collected", 0)),
        int(session.get("collected_count", 0)),
    )
    updated["most_rooms_world_completed"] = max(
        int(updated.get("most_rooms_world_completed", 0)),
        int(session.get("rooms_visited", 0)),
    )
    updated["timestamp_last_played"] = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    return updated


def fallback_session(engine: GameEngine, profile: dict[str, Any], reason: str) -> dict[str, Any]:
    """Build a best-effort session summary when the UI exits unexpectedly."""
    snapshot = engine.get_status_snapshot()
    player_name = str(profile.get("player_name") or "").strip() or "Player"
    return {
        "player_name": player_name,
        "collected_count": int(snapshot.get("collected_count", 0)),
        "rooms_visited": 1,
        "quit_reason": reason,
    }


def main() -> int:
    """Run the curses game, then update the profile."""
    args = parse_args()
    config_path = os.path.abspath(args.config)
    profile_path = os.path.abspath(args.profile)

    profile = load_profile(profile_path)
    engine = GameEngine(config_path)
    session: dict[str, Any] | None = None

    try:
        session = GameUI(engine).run(profile)
        return 0
    except Exception as exc:
        session = fallback_session(engine, profile, str(exc) or "Game ended unexpectedly")
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    finally:
        if session is not None:
            save_profile(profile_path, finalize_profile(profile, session))
        engine.destroy()


if __name__ == "__main__":
    raise SystemExit(main())
