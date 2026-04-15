# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

C++20 Pac-Man clone — a learning project intentionally exploring Modules, Concepts, and Coroutines where they genuinely solve problems. Not a prototype; production-quality code. See `docs/PROJECT.md` for the full vision, phased roadmap, and design principles.

## Build

Requires Visual Studio 2026 (MSVC 19.50+), CMake 3.28+, Windows x64.

```bash
# Configure (from repo root, using VS2026)
cmake -B build -G "Visual Studio 18 2026" -A x64

# Build
cmake --build build --config Debug
```

Alternatively, open in Visual Studio and use the `x64-Debug` CMake configuration (Ninja generator via `CMakeSettings.json`). Output lands in `out/build/x64-Debug/`.

SDL2 and ImGui are fetched automatically via `FetchContent` on first configure. SDL2.dll is copied to the output directory as a post-build step.

**Run:** Execute `pacman.exe`. Arrow keys move Pac-Man, `D` toggles the debug panel, ESC/close quits.

No automated tests — validate by playing and inspecting the debug panel.

## Architecture

```
main.cpp            Entry point — wiring only, intended as a "table of contents"
src/engine/         Cross-platform layer (SDL2, ImGui)
  renderer.ixx/cpp  Owns SDL_Window, SDL_Renderer, ImGui context (RAII)
  input.ixx/cpp     Stateless poll_input() → InputState snapshot per frame
  types.ixx         Vec2, AABB primitives
src/game/           Game logic
  stage.ixx/cpp     Game loop orchestrator — wires input → Pacman → render
  pacman.ixx/cpp    Pac-Man entity (satisfies GameEntity concept)
  map.ixx/cpp       Tile grid, collision checks, canonical maze layout
  debug.ixx/cpp     ImGui DebugView overlay (no Renderer dependency)
  concepts.ixx      Drawable, Updatable, Collidable, GameEntity concepts
  types.ixx         MAP_COLS, MAP_ROWS, TILE_SIZE, WINDOW_W/H constants
```

**No header files.** All modules use `.ixx` (interface unit, `export module`) and `.cpp` (implementation unit, `module;` only). The engine and game layers are strict: game imports engine, never the reverse.

**Entity system uses Concepts, not inheritance.** `GameEntity = Drawable && Updatable && Collidable`. A `static_assert` in `concepts.ixx` confirms `Pacman` satisfies the contract at compile time.

**Stage owns the sequence:** `main.cpp` drives a single `while` loop calling `poll_input → stage.update → stage.render`. Renderer and Input are passive.

**Coroutines are reserved for Phase 3 ghost AI** — don't apply them elsewhere.

## Coding Standards

Full rules in `docs/CODING_STANDARDS.md`. Key points:

- **Naming:** `PascalCase` types/concepts, `snake_case` functions/variables, `UPPER_SNAKE_CASE` constants, `snake_case_` member variables (trailing underscore), `lower.dot.separated` module names
- **Pointer alignment:** `Type* var` (type side)
- **Indentation:** 4 spaces, 100-char line limit
- **Module file structure:** global module fragment (`module;` + `#include`) → module declaration → `import` statements → exported code. No angle-bracket includes after the module declaration.
- **Class layout:** constructors → deleted copy/move → public methods → queries → private methods → members
- **Comments:** explain *why*, not *what*. Deferred work: `// TODO(phaseN):`
- **Error handling:** constructors throw `std::runtime_error`; methods return early

## Commit Convention

Every commit is tagged with a delegation tier that reflects authorship accountability:

- `[T1]` — Author writes independently, no AI
- `[T2]` — Author drives, AI navigates (pair programming)
- `[T3]` — AI drafts, author reviews and owns
- `[T4]` — Full delegation to AI

Use the appropriate tag on all commits. This is a deliberate part of the project's design, not optional metadata.
