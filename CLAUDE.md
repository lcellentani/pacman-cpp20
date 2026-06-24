# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Before starting any session

Read `docs/working_agreement.md` in full before doing anything else. It contains the
collaboration framework, delegation tiers, current phase entry conditions, architectural
decisions with their rationale, and standing constraints. Operating without it means
operating without the most important context in this project.

## Project Overview

A fully playable Pac-Man clone built with C++20 as a deliberate learning project. Every architectural decision maps to a specific C++20 feature: **Modules** for project structure, **Concepts** for the entity system, and **Coroutines** for ghost AI (planned). The goal is code that is technically defensible and feels good to play.

## Current Status

_Update this section at the end of every session — 3–5 lines, present tense._

Phase 3 is well underway. All four ghosts (Blinky, Pinky, Inky, Clyde) run coroutine-based
scatter/chase cycles with authentic arcade chase-target logic and house release governed by
dot-eaten thresholds and a force-release timer. `Frightened`/`Dead` exist in the `GhostState`
enum but have no coroutine path yet. For implementation detail (awaitables, tie-breaking,
house release specifics) see `docs/phase3_notes.md`.

## Open Items

_Update this list at the end of every session._

- Forced reversal on scatter↔chase switch is consumed at the next tile center (`reverse_pending_`), so a ghost mid-tile reverses up to one tile late — a deliberate simplification vs. the arcade's immediate flip
- `InputPoller` refactor deferred — edge detection still lives in `Stage` via `prev_debug_key_` / `prev_console_key_`
- `WallQuery`/`WorldQuery` concepts on hold until a second world query source exists
- Score display deferred to Phase 4 (`std::format`)
- `GhostState::Frightened` and `GhostState::Dead` exist in the enum and in `debug.cpp`'s display strings, but `Ghost::behavior()` has no coroutine path that enters either state yet

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

Known toolchain limitations (VS Code clang debugger struct-expansion, etc.):
`docs/troubleshooting.md`.

## Architecture

Module interface units use `.ixx`; implementation units use `.cpp`. The global module
fragment (`module;` + `#include`) is the only place external headers appear —
**`main.cpp` is not a module unit and cannot use a global module fragment**; `#include` goes
directly in the file body there.

For the current module graph and per-module architecture descriptions (the single source of
truth), see `docs/PROJECT.md` § Module graph / § Architecture.

## Coding Conventions

All conventions are in `docs/coding_standards.md`. Key rules:

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

## MCP Servers

- **github** — GitHub remote MCP server (user scope) for issue/PR/commit queries without leaving the session. Scoped to `pacman-cpp20`.

## Plans

Implementation plans are saved to `.claude/plans/` inside the project. When plan mode is
active, write the plan file there — not to the user-level `~/.claude/plans/` path that
the harness suggests by default. This keeps plans version-controlled alongside the code
they describe.

## Custom Commands

- `/build` — run the clang-ninja-debug preset build
- `/project_status` — report current phase, open items, and next action
- `/cpp-review <file>` — check a file against the project's C++ review criteria

## Hooks

Scripts live in `.claude/hooks/`. The following hooks have been designed but are currently inactive.

| Event | Trigger | Script | Effect |
|---|---|---|---|
| `PostToolUse` | `Write`, `Edit`, `MultiEdit` | `build-on-write.sh` | Runs `cmake --build --preset clang-ninja-debug`; filters output to `error:`/`warning:` lines (max 40) and feeds them back to Claude |
| `PreToolUse` | `Write`, `Edit`, `MultiEdit` | `guard-module-writes.sh` | **Inactive.** Designed to deny writes to `.ixx` files outside `src/`, enforcing the module layout constraint |

`build-on-write.sh` means Claude sees compiler diagnostics immediately after every file write and can self-correct without an explicit `/build` round-trip.

`guard-module-writes.sh` is inactive due to an upstream Claude Code bug — see
`docs/troubleshooting.md`.

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

Tag new commits appropriately. See `docs/working_agreement.md` for the full delegation framework.
