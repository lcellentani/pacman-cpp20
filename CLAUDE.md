# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A fully playable Pac-Man clone built with C++20 as a deliberate learning project. Every architectural decision maps to a specific C++20 feature: **Modules** for project structure, **Concepts** for the entity system, and **Coroutines** for ghost AI (planned). The goal is code that is technically defensible and feels good to play.

## Current Status

Phase 2 is near-complete. Two open items before Phase 3 can begin:
- Replace hardcoded `0.016f` timestep in `Stage::update` with real delta time
- Add `score_` state to `Stage` and wire pellet/super-pellet increment

Phase 3 target: coroutine ghost AI. Do not start ghost implementation until the delta time and score foundation are in place.

## Build

Toolchain-specific shell requirements:
- **MSVC presets** — run from an x64 Developer Command Prompt for VS 2026
- **Clang+Ninja presets** — run from Git Bash

```bash
cmake --preset <preset-name>
cmake --build --preset <preset-name>
```

Replace `<preset-name>` with a value from `CMakePresets.json`.

Dependencies (SDL2 2.30.2, ImGui 1.91.9b) are fetched automatically via FetchContent. There is no test framework and no linter — MSVC `/W4 /permissive-` strict mode is the quality gate. Concept satisfaction is asserted at compile time (e.g., `static_assert(GameEntity<Pacman>)`).

## Module Graph

```
main.cpp
└── Stage ──> Renderer, InputState
         └── Map, Pacman, DebugView
```

Module interface units use `.ixx`; implementation units use `.cpp`. The global module fragment (`module;` + `#include`) is the only place external headers (SDL2, ImGui, STL) appear.

## Architecture

**engine layer** — platform-facing, no game logic:
- `engine.renderer` — SDL2 window, pixel/rect/circle draw, ImGui frame management
- `engine.input` — polls SDL events, returns an `InputState` snapshot each frame
- `engine.types` — `AABB`, `Vec2`

**game layer** — logic, no SDL details:
- `game.concepts` — `Drawable`, `Updatable`, `Collidable`, `GameEntity` (all concept-based, no virtual dispatch)
- `game.types` — map dimensions (`MAP_COLS=28`, `MAP_ROWS=31`, `TILE_SIZE=24`), window size constants, `PacmanDebugState`, `Dir`
- `game.map` — 28×31 tile grid (`Wall`, `Pellet`, `SuperPellet`, `Empty`); hardcoded layout; queries by pixel or grid coords; `clear_tile()` for pellet collection
- `game.pacman` — tile-based movement with pixel-level offset accumulator; `queued_dir_` for buffered input; U-turns allowed immediately; direction changes only at tile centers
- `game.stage` — orchestrates Map + Pacman; pellet collection happens here when `pacman_entity_.is_at_tile_center()`
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
- **Phase 2** (near-complete) — Concepts-driven entity system, pellet collection
- **Phase 3** (planned) — Coroutine ghost AI (Blinky, Pinky, Inky, Clyde; scatter/chase/frightened/dead states)
- **Phase 4** (planned) — Lives, scoring with `std::format`, audio, high score persistence

## Commit Tags

Every commit is tagged to record the collaboration mode:
- `[T1]` Author drives, AI silent (core C++20 features, architecture)
- `[T2]` Author drives, AI navigates (mid-implementation questions)
- `[T3]` AI drafts, author owns (boilerplate, CMake, skeletons)
- `[T4]` Full delegation (compiler errors, docs, repetitive patterns)

Tag new commits appropriately. See `docs/PROJECT.md` for the full delegation framework.
