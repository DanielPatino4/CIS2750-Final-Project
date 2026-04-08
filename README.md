# Treasure Runner

Treasure Runner is a terminal-based adventure game built with a C backend and a Python interface.
The project demonstrates systems programming, cross-language integration, modular game architecture, and automated testing.

## Project Highlights

- Designed and implemented a modular C game engine (`player`, `room`, `graph`, and `world_loader` components)
- Exposed C engine functionality to Python through structured bindings
- Built an interactive terminal UI for gameplay flow, profile updates, and status rendering
- Added support for world configuration loading via `.ini` assets
- Implemented unit tests for C modules and Python integration behavior

## Tech Stack

- **Languages:** C, Python
- **Build Tools:** Make, GCC
- **Testing:** Check (C unit tests), Python test suite
- **Runtime:** Linux (validated in Ubuntu dev container)

## Repository Structure

- `c/` — core engine implementation, headers, and C unit tests
- `python/` — Python runner, bindings, models, and terminal UI
- `assets/` — starter world configuration and game assets
- `dist/` — runtime shared libraries required by the project

## Quick Start

From the repository root:

```bash
make dist
source env.sh
python3 python/run_game.py --config assets/starter.ini --profile test_player
```

## Running Tests

### C Unit Tests

```bash
make -C c test
```

### Python Tests

```bash
python3 -m pytest python/tests
```

## Engineering Focus

This project emphasizes:

- clear API boundaries between C and Python layers
- memory-safe practices and explicit lifecycle management
- reproducible local builds with Make-based workflows
- maintainable, testable module design

## Author

Daniel Patino



