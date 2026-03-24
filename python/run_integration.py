#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

# below are the imports that are in the instructors solution.
# you may need different ones.  Use them if you wish.
import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


def get_state(engine):
    room_id = engine.player.get_room()
    pos_x, pos_y = engine.player.get_position()
    collected = engine.player.get_collected_count()
    return (room_id, pos_x, pos_y, collected)


def state_to_str(state):
    room_id, pos_x, pos_y, collected = state
    return f"room={room_id}|x={pos_x}|y={pos_y}|collected={collected}"


def write_line(handle, event_name, **fields):
    parts = [event_name]
    for key, value in fields.items():
        parts.append(f"{key}={value}")
    handle.write("|".join(parts) + "\n")


def find_entry_direction(engine):
    trial_order = [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]

    for direction in trial_order:
        before = get_state(engine)
        try:
            engine.move_player(direction)
        except ImpassableError:
            continue
        except GameError:
            engine.reset()
            continue

        after = get_state(engine)
        same_room = before[0] == after[0]
        moved_pos = (before[1], before[2]) != (after[1], after[2])
        if same_room and moved_pos:
            engine.reset()
            return direction

        engine.reset()

    raise RuntimeError("No valid entry direction found")


def run_sweep(engine, handle, phase_name, direction, step):
    write_line(handle, "SWEEP_START", phase=phase_name, dir=direction.name)

    seen = {get_state(engine)}
    successful_moves = 0
    reason = "BLOCKED"

    while True:
        before = get_state(engine)
        try:
            engine.move_player(direction)
            after = get_state(engine)

            if after == before:
                step += 1
                write_line(
                    handle,
                    "MOVE",
                    step=step,
                    phase=phase_name,
                    dir=direction.name,
                    result="NO_PROGRESS",
                    before=state_to_str(before),
                    after=state_to_str(after),
                    delta_collected=0,
                )
                reason = "BLOCKED"
                break

            step += 1
            delta_collected = after[3] - before[3]
            write_line(
                handle,
                "MOVE",
                step=step,
                phase=phase_name,
                dir=direction.name,
                result="OK",
                before=state_to_str(before),
                after=state_to_str(after),
                delta_collected=delta_collected,
            )
            successful_moves += 1

            if after in seen:
                reason = "CYCLE_DETECTED"
                break
            seen.add(after)

        except ImpassableError:
            step += 1
            write_line(
                handle,
                "MOVE",
                step=step,
                phase=phase_name,
                dir=direction.name,
                result="BLOCKED",
                before=state_to_str(before),
                after=state_to_str(before),
                delta_collected=0,
            )
            reason = "BLOCKED"
            break
        except GameError:
            step += 1
            write_line(
                handle,
                "MOVE",
                step=step,
                phase=phase_name,
                dir=direction.name,
                result="ERROR",
                before=state_to_str(before),
                after=state_to_str(before),
                delta_collected=0,
            )
            reason = "BLOCKED"
            break

    write_line(handle, "SWEEP_END", phase=phase_name, reason=reason, moves=successful_moves)
    return step


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)

    engine = GameEngine(config_path)
    try:
        with open(log_path, "w", encoding="utf-8") as handle:
            room_count = engine.get_room_count()
            room_width, room_height = engine.get_room_dimensions()
            write_line(
                handle,
                "RUN_START",
                config=config_path,
                rooms=room_count,
                room_width=room_width,
                room_height=room_height,
            )

            spawn_state = get_state(engine)
            write_line(handle, "STATE", step=0, phase="SPAWN", state=state_to_str(spawn_state))

            entry_direction = find_entry_direction(engine)
            write_line(handle, "ENTRY", direction=entry_direction.name)

            engine.reset()
            step = 1
            before = get_state(engine)
            try:
                engine.move_player(entry_direction)
                after = get_state(engine)
                result = "OK" if after != before else "NO_PROGRESS"
                delta = after[3] - before[3]
                write_line(
                    handle,
                    "MOVE",
                    step=step,
                    phase="ENTRY",
                    dir=entry_direction.name,
                    result=result,
                    before=state_to_str(before),
                    after=state_to_str(after),
                    delta_collected=delta,
                )
            except GameError:
                write_line(
                    handle,
                    "MOVE",
                    step=step,
                    phase="ENTRY",
                    dir=entry_direction.name,
                    result="ERROR",
                    before=state_to_str(before),
                    after=state_to_str(before),
                    delta_collected=0,
                )
                handle.write("TERMINATED: Initial Move Error\n")
                final_state = get_state(engine)
                write_line(handle, "RUN_END", steps=step, collected_total=final_state[3])
                return 0

            sweeps = [
                ("SWEEP_SOUTH", Direction.SOUTH),
                ("SWEEP_WEST", Direction.WEST),
                ("SWEEP_NORTH", Direction.NORTH),
                ("SWEEP_EAST", Direction.EAST),
            ]
            for phase, direction in sweeps:
                step = run_sweep(engine, handle, phase, direction, step)

            final_state = get_state(engine)
            write_line(handle, "STATE", step=step, phase="FINAL", state=state_to_str(final_state))
            write_line(handle, "RUN_END", steps=step, collected_total=final_state[3])
    finally:
        engine.destroy()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
