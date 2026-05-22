# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A fully playable Pac-Man clone built with C++20 as a deliberate learning project. Every architectural decision maps to a specific C++20 feature: **Modules** for project structure, **Concepts** for the entity system, and **Coroutines** for ghost AI (planned). The goal is code that is technically defensible and feels good to play.

## Current Status

Phase 2 is complete. Phase 3 (coroutine ghost AI) is the active next target.

Notable Phase 2 additions:
- Real delta time via `std::chrono::steady_clock` in `main.cpp`
- `game.config` — `GameConfig` struct with live ImGui tweaking and nlohmann/json persistence (`config.json` next to the executable)

## Build

Toolchain-specific shell requirements:
- **MSVC presets** — run from an x64 Developer Command Prompt for VS 2026
- **Clang+Ninja presets** — run from Git Bash

```bash
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

Replace `<preset-name>` with a value from `CMakePresets.json`.

Dependencies (SDL2 2.30.2, ImGui 1.91.9b, nlohmann/json 3.11.3) are fetched automatically via FetchContent. There is no test framework and no linter — MSVC `/W4 /permissive-` strict mode is the quality gate. Concept satisfaction is asserted at compile time (e.g., `static_assert(GameEntity<Pacman>)`).

If the Clang/Ninja build fails with "user-mapped section open" errors on `.pcm` files, the IDE has them locked — use `--clean-first` to recover.

## Module Graph

The following graph is intentionally high-level and the authoritative reference is the `.ixx` files themselves.

```
main.cpp
└── Stage ──> Renderer, InputState, GameConfig
         └── Map, Pacman, DebugView
```

Module interface units use `.ixx`; implementation units use `.cpp`. The global module fragment (`module;` + `#include`) is the only place external headers appear — **`main.cpp` is not a module unit and cannot use a global module fragment**; `#include` goes directly in the file body there.

## Architecture

**engine layer** — platform-facing, no game logic:
- `engine.renderer` — SDL2 window, pixel/rect/circle draw, ImGui frame management
- `engine.input` — polls SDL events, returns an `InputState` snapshot each frame
- `engine.types` — `AABB`, `Vec2`

**game layer** — logic, no SDL details:
- `game.config` — `GameConfig` struct (runtime-mutable values: speeds, timers); `load_config` / `save_config` using nlohmann/json exception-free API
- `game.concepts` — `Drawable`, `Updatable`, `Collidable`, `GameEntity` (all concept-based, no virtual dispatch)
- `game.types` — map dimensions (`MAP_COLS=28`, `MAP_ROWS=31`, `TILE_SIZE=24`), window size constants, `PacmanDebugState`, `Dir`
- `game.map` — 28×31 tile grid (`Wall`, `Pellet`, `SuperPellet`, `Empty`); hardcoded layout; queries by pixel or grid coords; `clear_tile()` for pellet collection
- `game.pacman` — tile-based movement with pixel-level offset accumulator; `queued_dir_` for buffered input; U-turns allowed immediately; direction changes only at tile centers
- `game.stage` — orchestrates Map + Pacman; pellet collection and score increment happen here when `pacman_entity_.is_at_tile_center()`
- `game.debug` — ImGui panel (toggle with `D`); shows position, velocity, AABB, minimap

**main.cpp** — minimal loop: `poll_input → stage.update → stage.render`.

## Coding Conventions

All conventions are in `docs/CODING_STANDARDS.md`. Key rules:

| Thing | Style |
|---|---|
| Types, concepts | `PascalCase` |
| Functions, variables | `snake_case` |
| Member variables | `trailing_underscore_` |
| Constants | `UPPER_SNAKE_CASE` |
| Enum values | `PascalCase` |
| Module names | `lower.dot.separated` |
| Indentation | 4 spaces |
| Max line length | 100 chars |

**Do not use** virtual functions or inheritance in the entity system — use Concepts. Do not use `import <...>` (angle-bracket header units) — put `#include` in the global module fragment. Do not use `auto` as a return type except for coroutines and lambdas.

## Development Phases

- **Phase 1** (complete) — Modules, SDL2 window, tile map, basic Pac-Man movement
- **Phase 2** (complete) — Concepts-driven entity system, pellet collection, delta time, GameConfig
- **Phase 3** (planned) — Coroutine ghost AI (Blinky, Pinky, Inky, Clyde; scatter/chase/frightened/dead states)
- **Phase 4** (planned) — Lives, scoring with `std::format`, audio, high score persistence

## Commit Tags

Every commit is tagged to record the collaboration mode:
- `[T1]` Author drives, AI silent (core C++20 features, architecture)
- `[T2]` Author drives, AI navigates (mid-implementation questions)
- `[T3]` AI drafts, author owns (boilerplate, CMake, skeletons)
- `[T4]` Full delegation (compiler errors, docs, repetitive patterns)

Tag new commits appropriately. See `docs/PROJECT.md` for the full delegation framework.
