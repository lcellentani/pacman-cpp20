# Phase 3 Implementation Notes — Coroutine Ghost AI

Detailed reference for Phase 3 (coroutine-based ghost AI). Split out of `CLAUDE.md` because
it's implementation-level detail, not something every session needs loaded — consult this
file when working on ghost behavior, the coroutine scheduler, or the concept/debug-state
plumbing that supports them.

## Current implementation state

All four ghosts (Blinky, Pinky, Inky, Clyde) are implemented with coroutine-based
scatter/chase cycles using `wait_for` and `walk_path` awaitables. All four chase targets
follow authentic arcade logic, including the up-facing overflow bug shared by Pinky and Inky
(`arcade_ahead`), deterministic tie-breaking via the priority-ordered `cardinal_dirs()`
(Up > Left > Down > Right), and a forced direction reversal on each scatter↔chase switch
(`enter_phase`/`reverse_pending_`). `game.scheduler`, `engine.log`, `engine.random`, and
`game.console` support ghost AI and in-game logging. `GhostDebugState` and `GameState` are
defined in `game.types` and wired up; `Ghost` tracks its real `state_` (`House`, `Scatter`,
`Chase`; `Frightened`/`Dead` exist in the enum but have no coroutine path yet) and
`debug_state()` reports it accurately.

Ghost house release logic is wired up: each non-Blinky ghost waits in `GhostState::House` via
the `wait_for_release` awaitable until a per-ghost dot-eaten threshold or a force-release
timer fires (`ghost_release_order()`: Pinky → Inky → Clyde), then exits via a scripted
`walk_path`. `Stage` tracks `dots_eaten`/`dot_timer` on `GameState` (`Stage::eat_dot`),
`GameConfig` exposes `ghost_speed` and the house thresholds/timer as live-tweakable sliders in
`game.debug`, and `game.console` gained `dots_eaten` / `dot_timer` / `next_force_release`
inspection commands.

The executable uses `/SUBSYSTEM:WINDOWS` (no console window) with `/ENTRY:mainCRTStartup`, so
`int main()` remains the entry point with no `WinMain` shim. `SDL2::SDL2main` is not linked.

## Codebase archaeology (verified entering Phase 3)

Findings from a pre-Phase-3 audit, kept here as a verified snapshot of the boundaries ghost AI
had to respect.

### Module graph (verified)

```
main.cpp
└── import game.stage
         ├── engine.renderer
         ├── engine.input
         ├── game.concepts
         ├── game.config
         ├── game.console
         ├── game.debug
         ├── game.ghost
         ├── game.map
         ├── game.pacman
         ├── game.scheduler
         └── game.types

game.ghost imports:
         ├── engine.log
         ├── engine.random
         ├── engine.renderer
         ├── engine.types
         ├── game.concepts
         ├── game.map
         ├── game.scheduler
         └── game.types

game.scheduler imports:
         └── game.concepts
```

`main.cpp` is the only consumer of `game.stage`. Ghost modules are wired as imports inside
`stage.ixx` — not in `main.cpp`. For the current, authoritative module graph see
`docs/PROJECT.md`.

### SDL boundary (verified)

SDL calls are fully contained in the `engine` layer:
- `src/engine/renderer.cpp` — init, window, renderer, draw calls, present; calls
  `SDL_SetMainReady()` before `SDL_Init`
- `src/engine/input.cpp` — `SDL_PollEvent`, `SDL_GetKeyboardState`
- `src/engine/renderer.ixx` — `SDL_Window*`, `SDL_Renderer*` member declarations

Every global module fragment that includes `<SDL.h>` must define `SDL_MAIN_HANDLED` first:
```cpp
module;
#define SDL_MAIN_HANDLED
#include <SDL.h>
```
This prevents `SDL_main.h` from macro-redefining `main` as `SDL_main`. Required in
`renderer.cpp`, `renderer.ixx`, and `input.cpp`. Any future engine module including SDL
headers must follow the same pattern.

No SDL symbols appear in any `game.*` module. Ghost implementation must stay on the game side
of this boundary.

### Concept system (verified)

All concepts defined in `src/game/concepts.ixx`:

| Concept | Requires |
|---|---|
| `Drawable<T>` | `t.draw(r) -> void` |
| `Updatable<T>` | `t.update(dt) -> void` |
| `Collidable<T>` | `t.get_bounds() -> AABB` |
| `Controllable<T>` | `t.handle_input(input) -> void` |
| `FrameCallback<F>` | `f(dt) -> void` |
| `GameEntity<T>` | `Drawable && Updatable && Collidable` (composed) |

`Ghost` satisfies `Drawable` and `Collidable` but deliberately does **not** satisfy
`Updatable`. Ghost per-frame work is driven by `Scheduler`-registered coroutine awaitables —
there is no `update(float dt)` entry point on `Ghost`. Do not add one to make the concept fit;
that would break the coroutine model. Do not add `static_assert(GameEntity<Ghost>, ...)` — the
constraint does not apply to coroutine-driven entities.

### Debug state pattern

`GhostDebugState` (defined in `game.types`) follows the `PacmanDebugState` pattern. It carries
`id`, `coord`, `dir_x/y`, `speed`, `bounds`, `color`, `state`, `target`, and `path`.
`Ghost::debug_state()` is implemented and reports the ghost's real `state_` (`House`/
`Scatter`/`Chase` are reachable; `Frightened`/`Dead` are not yet entered by any coroutine path
— see `CLAUDE.md`'s Open Items).
