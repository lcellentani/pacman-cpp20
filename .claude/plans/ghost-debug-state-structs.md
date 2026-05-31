# Plan: Ghost Debug State Structs

## Context

Phase 3 entry requires ghost debug state structs to be designed before any ghost rendering
is implemented. WORKING_AGREEMENT §Phase 3 open items: "Ghost entities will each need a
debug state struct following the `PacmanDebugState` pattern; design this before implementing
ghost rendering."

`PacmanDebugState` lives in `game.types` and is a plain data struct returned by
`Pacman::debug_state()`. `DebugView` consumes it by value; `Stage` calls
`pacman_entity_.debug_state()` and passes it straight through. The struct has no
behaviour — it is a snapshot of what the debug panel needs to render.

---

## Answer: one struct, not four

Four separate structs (`BlinkyDebugState`, `PinkyDebugState`, …) would be the wrong call.
All four ghosts expose the same *category* of debug data:

- tile position
- direction
- speed + bounds
- current AI state (scatter / chase / frightened / dead)
- current target tile

The computation of the target tile differs per personality (Blinky does direct chase,
Pinky looks 4 tiles ahead, etc.), but the *result* is the same type: a `{col, row}` pair.
A per-personality struct would carry identical fields — the only difference would be the
type name. That is not a useful distinction in a debug overlay.

The WORKING_AGREEMENT says each ghost entity needs a debug state *method* following the
`PacmanDebugState` pattern — not a distinct *type* per ghost. One shared
`GhostDebugState` is the right read.

---

## New types to add to `game.types` (`src/game/types.ixx`)

### `GhostState` enum

```cpp
export enum class GhostState { Scatter, Chase, Frightened, Dead };
```

This is the AI state tag returned by the coroutine. Four values — matches Phase 3 design
exactly.

### `GhostId` enum

```cpp
export enum class GhostId { Blinky, Pinky, Inky, Clyde };
```

Carries ghost identity into the debug panel without coupling DebugView to any ghost class.
Also needed later when Stage holds four ghost instances and DebugView needs to label each
section.

### `GhostDebugState` struct

```cpp
export struct GhostDebugState {
    GhostId    id;
    int        col, row;
    int        dir_x, dir_y;
    float      speed;
    AABB       bounds;
    GhostState state;
    int        target_col, target_row;
};
```

Fields mirror `PacmanDebugState` exactly where the data is the same (`col`, `row`,
`dir_x`, `dir_y`, `speed`, `bounds`), then add the two ghost-specific fields: `state`
and `target_{col,row}`.

`target_col / target_row` default to the ghost's scatter corner when in Scatter state, to
the computed chase tile when in Chase, and are irrelevant (can be -1/-1 sentinel) in
Frightened and Dead. The debug panel can choose to display or suppress them.

---

## Impact on `DebugView` (`src/game/debug.ixx` / `debug.cpp`)

`DebugView::draw()` currently takes `(const Map&, const PacmanDebugState&, GameConfig&)`.

Extend the signature to accept a span of ghost states:

```cpp
void draw(const Map&, const PacmanDebugState&,
          std::span<const GhostDebugState>, GameConfig&);
```

Add a new private helper: `draw_ghost_section(const GhostDebugState&)`.

Stage would call it as:

```cpp
std::array<GhostDebugState, 4> ghost_states = { /* from each ghost */ };
debug_.draw(map_, pacman_entity_.debug_state(), ghost_states, config_);
```

For now (no ghosts implemented yet) this signature is *prepared* but not yet populated.
The array can be left empty / zero-initialised until Phase 3 ghost classes exist.

---

## Files to modify

| File | Change |
|---|---|
| `src/game/types.ixx` | Add `GhostState`, `GhostId`, `GhostDebugState` |
| `src/game/debug.ixx` | Extend `draw()` signature; add `draw_ghost_section()` declaration |
| `src/game/debug.cpp` | Add `draw_ghost_section()` stub (empty body — nothing to render yet) |
| `src/game/stage.cpp` | Update `render()` call to pass empty `ghost_states` array |

`src/game/stage.ixx` — no change needed; the public interface of Stage does not expose
ghost state directly.

---

## What this does NOT do

- Does not implement any ghost class
- Does not wire any coroutine
- Does not add ghost rendering
- Does not add ghost speed/scatter-timer fields to `GameConfig` (those belong in a later
  step when the coroutine code needs to consume them)

---

## Verification

Build must stay clean after changes:
```bash
cmake --build --preset clang-ninja-debug
```

Expected: no new errors; the two pre-existing warnings (`-Wreorder-ctor` in `stage.cpp`,
`found both wmain and main` linker warning) may still appear — they are not regressions.

Runtime: launch the game, press `D` — the debug panel should open as before. No ghost
section is visible yet (stub body is empty), which is the correct baseline state.
