# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Before starting any session

Read `docs/WORKING_AGREEMENT.md` in full before doing anything else. It contains the
collaboration framework, delegation tiers, current phase entry conditions, architectural
decisions with their rationale, and standing constraints. Operating without it means
operating without the most important context in this project.

## Project Overview

A fully playable Pac-Man clone built with C++20 as a deliberate learning project. Every architectural decision maps to a specific C++20 feature: **Modules** for project structure, **Concepts** for the entity system, and **Coroutines** for ghost AI (planned). The goal is code that is technically defensible and feels good to play.

## Current Status

_Update this section at the end of every session — 3–5 lines, present tense._

Phase 2 complete. Phase 3 (coroutine ghost AI) is the active next target. Tile-aligned movement, pellet collection, delta time, ImGui debug panel, and `game.config` with nlohmann/json persistence are all in place. No ghost logic yet — that is the Phase 3 starting point.

Two pre-existing compiler warnings: `-Wreorder-ctor` in `stage.cpp:5` (member init order mismatch) and `found both wmain and main` in the linker (harmless, main is used). Neither is blocking Phase 3.

## Open Items

_Update this list at the end of every session._

- Ghost entities: no implementation yet — Phase 3 starting point
- `InputPoller` refactor deferred to Phase 3; edge detection currently lives in `Stage`
- `WallQuery`/`WorldQuery` concepts on hold until a second world query source exists
- Ghost debug state structs: design before implementing ghost rendering (follow `PacmanDebugState` pattern)
- Score display deferred to Phase 4 (`std::format`)

## Phase 3 Context

Reference findings from codebase archaeology before starting ghost AI work.

### Module graph (verified)

```
main.cpp
└── import game.stage
         ├── engine.renderer
         ├── engine.input
         ├── game.concepts
         ├── game.config
         ├── game.debug
         ├── game.map
         └── game.pacman
```

`main.cpp` is the only consumer of `game.stage`. Ghost modules (`game.ghost` or similar) will be added as imports inside `stage.ixx`, not in `main.cpp`.

### SDL boundary (verified)

SDL calls are fully contained in the `engine` layer:
- `src/engine/renderer.cpp` — init, window, renderer, draw calls, present
- `src/engine/input.cpp` — `SDL_PollEvent`, `SDL_GetKeyboardState`
- `src/engine/renderer.ixx` — `SDL_Window*`, `SDL_Renderer*` member declarations

No SDL symbols appear in any `game.*` module. Ghost implementation must stay on the game side of this boundary.

### Concept system (verified)

All concepts defined in `src/game/concepts.ixx`:

| Concept | Requires |
|---|---|
| `Drawable<T>` | `t.draw(r) -> void` |
| `Updatable<T>` | `t.update(dt) -> void` |
| `Collidable<T>` | `t.get_bounds() -> AABB` |
| `Controllable<T>` | `t.handle_input(input) -> void` |
| `GameEntity<T>` | `Drawable && Updatable && Collidable` (composed) |

Ghost types must satisfy `GameEntity`. Add a `static_assert(GameEntity<Ghost>, ...)` in the ghost module interface, following the pattern at `src/game/pacman.ixx:52`.

### Debug state pattern

Follow `PacmanDebugState` (defined in `game.types`) when designing ghost debug structs. Design the ghost debug state struct before implementing ghost rendering — this is an open item.

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

Code review criteria: `.claude/skills/cpp-review.md`

**Do not use** virtual functions or inheritance in the entity system — use Concepts. Do not use `import <...>` (angle-bracket header units) — put `#include` in the global module fragment. Do not use `auto` as a return type except for coroutines and lambdas.

## Custom Commands

- `/build` — run the clang-ninja-debug preset build
- `/project_status` — report current phase, open items, and next action
- `/cpp-review <file>` — check a file against the project's C++ review criteria

## Hooks

Configured in `.claude/settings.json`; scripts live in `.claude/hooks/`.

| Event | Trigger | Script | Effect |
|---|---|---|---|
| `PostToolUse` | `Write`, `Edit`, `MultiEdit` | `build-on-write.sh` | Runs `cmake --build --preset clang-ninja-debug`; filters output to `error:`/`warning:` lines (max 40) and feeds them back to Claude |
| `PreToolUse` | `Write`, `Edit`, `MultiEdit` | `guard-module-writes.sh` | **Inactive — see note below.** designed to deny writes to `.ixx` files outside `src/`, enforcing the module layout constraint |

`build-on-write.sh` means Claude sees compiler diagnostics immediately after every file write and can self-correct without an explicit `/build` round-trip.

> **Known limitation — `guard-module-writes.sh` is not active.**
> The script is wired to a `PreToolUse` event with `permissionDecision: "deny"`, but this is blocked by Claude Code bug [#37210](https://github.com/anthropics/claude-code/issues/37210) (filed March 2026, still open): `permissionDecision: "deny"` is honoured for `Bash` tool calls but **silently ignored for `Edit` and `Write`** — the file is modified anyway. The hook script exists and is correct; re-enable it in `settings.json` once the bug is resolved.

## Development Phases

- **Phase 1** (complete) — Modules, SDL2 window, tile map, basic Pac-Man movement
- **Phase 2** (complete) — Concepts-driven entity system, pellet collection, delta time, GameConfig
- **Phase 3** (active) — Coroutine ghost AI (Blinky, Pinky, Inky, Clyde; scatter/chase/frightened/dead states)
- **Phase 4** (planned) — Lives, scoring with `std::format`, audio, high score persistence

## Commit Tags

Every commit is tagged to record the collaboration mode:
- `[T1]` Author drives, AI silent (core C++20 features, architecture)
- `[T2]` Author drives, AI navigates (mid-implementation questions)
- `[T3]` AI drafts, author owns (boilerplate, CMake, skeletons)
- `[T4]` Full delegation (compiler errors, docs, repetitive patterns)

Tag new commits appropriately. See `docs/WORKING_AGREEMENT.md` for the full delegation framework.
