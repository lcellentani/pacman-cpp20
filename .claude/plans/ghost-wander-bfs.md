# BFS navigation for the random-wander placeholder (keep greedy for Phase 3)

## Context

Ghost wander navigation uses **pure greedy one-tile-lookahead** in
`Ghost::move_toward_greedy` (`src/game/ghost.cpp`): at each tile center it picks the
neighbour minimising squared Euclidean distance to a fixed random target, with a no-reverse
rule. Greedy hill-climbing is **incomplete**: around the walkable ring encircling the ghost
house there are target positions where, at every junction, "continue around the ring" is
locally closer than "branch off," and the no-reverse rule locks the ghost into a stable
cycle. With a **static** target nothing perturbs the cycle and there is no timeout, so the
ghost circles forever and `move_to` never completes.

**Important:** `move_toward_greedy` (greedy 1-tile-lookahead, min-Euclidean, no-reverse) is
*the authentic arcade ghost algorithm* and will be the core of Phase-3 chase/scatter (only
the per-ghost target tile differs). The loop bug comes from the **static random target** — a
temporary placeholder behaviour — not from the algorithm. So we **keep greedy** and only
change how the *placeholder wander* navigates.

Decision: drive the random-wander with **BFS shortest-path** (complete, never loops, cheap:
≤868 tiles, run once per target). `move_toward_greedy` / `move_to` / `can_move` are
**retained, marked as Phase-3 chase/scatter machinery**, currently unused by wander.

## Key insight — minimal new code

The existing `walk_path` awaiter already walks a `std::span<const MapCoord>` of adjacent
tiles by calling `Ghost::move_toward(next_tile, dt)` and advancing on `ghost_reached`. A BFS
path is exactly such a sequence, so we **reuse `walk_path` and `move_toward` unchanged** —
only the path *source* changes. No new awaiter/movement code.

## Changes

### 1. `Map::find_path` — new BFS helper (`src/game/map.ixx` + `map.cpp`)

Grid topology belongs in `Map`. Add:

```cpp
[[nodiscard]] std::vector<MapCoord> find_path(MapCoord from, MapCoord to) const;
```

BFS implementation:
- Walkability = existing `is_wall_at(col, row)` (blocks `Wall` + `Door`; `GhostHouse` stays
  unreachable behind the door — consistent with `Ghost::can_move`).
- `std::array<int, MAP_ROWS*MAP_COLS> parent` filled with `-1`; `std::queue<int>` frontier;
  index = `row * MAP_COLS + col`. Expand the 4 canonical dirs, bounds-check, skip walls and
  visited; stop when `to` is reached.
- `from == to` → return `{ to }`; `to` unreachable → return `{}`.
- Reconstruct goal→start via `parent`, **exclude the start tile**, `std::reverse`. Result is
  the `from`-exclusive … `to`-inclusive sequence of adjacent tiles.
- `MapCoord` is `{ col, row }` → build as `{ idx % MAP_COLS, idx / MAP_COLS }`.

Includes: add `<vector>` to `map.ixx` global fragment (return type); add `<vector>`,
`<queue>`, `<algorithm>` to `map.cpp` (`<array>` already present).

### 2. `Ghost::behavior` — wander via BFS (`src/game/ghost.cpp`)

```cpp
Task Ghost::behavior(Scheduler& scheduler) {
    if (id_ != GhostId::Blinky) {
        co_await walk_path(*this, scheduler, path_for_ghost(id_));
    }
    while (true) {
        target_ = pick_random_target();
        path_ = map_->find_path({ col_, row_ }, target_);   // owned by Ghost, stable across await
        if (path_.empty()) {
            log_warn("ghost: no path to target, re-rolling");
            continue;   // not reachable in a fully-connected maze; guards against spin
        }
        co_await walk_path(*this, scheduler, path_);         // span over path_
    }
}
```

`path_` is a new `Ghost` member so its storage outlives the suspended `co_await`
(`walk_path` holds only a `std::span` into it). It is reassigned only *after* the prior await
returns, so there is no aliasing during the await.

### 3. Retain greedy, mark it for Phase 3 (no deletion)

- **Keep** `move_toward_greedy`, `can_move`, and the `move_to` awaiter struct exactly as they
  are. Add a one-line comment above `move_to` / `move_toward_greedy`, e.g.:
  `// Retained for Phase 3 chase/scatter — authentic arcade greedy targeting. Wander uses BFS.`
- **Keep** the imports/includes they need: `import engine.random;`, `#include <limits>`,
  `#include <cassert>`. No pruning.
- These are member/file-local definitions, so being temporarily uncalled produces no
  `-Wunused-function` warnings under `/W4` (that diagnostic targets free/static functions).

### 4. Share the cardinal-directions array (`src/game/types.ixx`)

The 4-direction array is duplicated in `move_toward_greedy` and would be triplicated in
`find_path`. `Dir` lives in `game.types`, visible to both `game.ghost` and `game.map`. Add a
span accessor:

```cpp
export [[nodiscard]] inline std::span<const Dir> cardinal_dirs() {
    static constexpr std::array<Dir, 4> dirs = { { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } } };
    return dirs;   // span over a static — valid for the program lifetime
}
```

- `move_toward_greedy` (ghost.cpp): drop its local array, iterate `cardinal_dirs()`.
- `find_path` (map.cpp): iterate `cardinal_dirs()`.
- types.ixx global fragment: add `<array>`, `<span>`.

### 5. Debug visualization of the BFS path

Expose the path through `GhostDebugState` and render it as a faded polyline through tile
centers on the minimap (ghost color, alpha 120), starting at the ghost so it connects to the
route. Confirms BFS threads the maze instead of looping.

- `types.ixx`: add `std::span<const MapCoord> path;` as the last field of `GhostDebugState`.
- `Ghost::debug_state()` (ghost.cpp): append `path_` to the returned aggregate.
- `DebugView::draw_map_section` (debug.cpp): in the existing per-ghost loop, draw
  `AddLine` segments from `ghost.coord` through each `ghost.path` tile center. Add `<span>` to
  debug.cpp's fragment only if the compiler requires it.

## Files to change

- `src/game/types.ixx` — `cardinal_dirs()`; `path` field on `GhostDebugState`; +`<array>`,`<span>`.
- `src/game/map.ixx` — declare `find_path`; add `<vector>`.
- `src/game/map.cpp` — implement `find_path` (BFS, uses `cardinal_dirs()`); add `<vector>`,
  `<queue>`, `<algorithm>`.
- `src/game/ghost.ixx` — add `std::vector<MapCoord> path_;` member + `<vector>` include.
  (No removals — greedy declarations stay.)
- `src/game/ghost.cpp` — rewrite `behavior` to use `find_path` + `walk_path`;
  `move_toward_greedy` iterates `cardinal_dirs()`; `debug_state()` appends `path_`; add the
  retained-for-Phase-3 comment. Greedy logic otherwise untouched.
- `src/game/debug.cpp` — draw the BFS path polyline in `draw_map_section`.

## What does NOT change

- `walk_path` awaiter and `move_toward` — reused verbatim.
- House-exit scripting (`path_for_ghost` + `walk_path`) — unchanged.
- `move_toward_greedy`, `move_to`, `can_move` — kept for Phase 3.
- `is_wall_at` / Door / GhostHouse semantics, `Scheduler`, the `Updatable` concept.

## Verification

1. `/build` — compile clean under strict flags.
2. `/run`, toggle debug (D), observe ~60 s:
   - All four ghosts navigate to random targets across the **whole** map (incl. far corners),
     not just near the house.
   - **No ghost circles the ghost-house ring.** Each `target` marker is reached and a fresh
     target appears every few seconds for every ghost.
   - Reproduce the prior case: a ghost at the bottom-left with a top-right target now threads
     the maze straight there instead of looping.
3. Leave running 2–3 min — no ghost parks or circles indefinitely.
