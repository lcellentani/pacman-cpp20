# MapCoord type + Map random-coordinate API

> On approval, move to `.claude/plans/mapcoord-and-random.md` in the repo (per CLAUDE.md plan-location convention).

## Context

`Ghost` currently models a map coordinate as `std::pair<int, int>` — see `ghost.ixx:23-25` (`pick_random_target`, `move_toward`, `ghost_reached`) and the `move_to` awaiter at `ghost.cpp:12-37`. The pair has no field names, no operator equality, and it doesn't compose with `Map`'s int-pair API (`tile_at_index(int row, int col)`, `clear_tile(int col, int row)`, `is_wall_at(int col, int row)`) which uses inconsistent argument orders. As Phase 3 ghost AI grows (frightened-mode random scatter, target picking for Inky/Pinky, debug overlays), this representation will multiply.

Concurrently, `Ghost::pick_random_target()` is stubbed (`ghost.cpp:54-57`) returning `{0,0}` — a real implementation needs a random walkable tile from the `Map`, which requires both an RNG (none in the project yet — confirmed by grep for `mt19937`/`random_device`/`srand`) and a new `Map` method.

Outcome:
- New `MapCoord { int col, row; }` struct used by Ghost, by new Map overloads, and by the existing debug-state structs.
- New `engine.random` module hosting a process-global seeded `std::mt19937` and helpers.
- New `Map::pick_random_walkable() -> MapCoord` returning a uniform random non-Wall tile (Empty / Pellet / SuperPellet all qualify; ghost-house interior is included because all 'G' cells map to `Tile::Empty`).
- `Pacman` gains a `current_coord()` accessor; its internal `col_`/`row_` int members stay (out of scope to refactor movement).

## Design

### 1. `MapCoord` in `game.types`

`src/game/types.ixx` — add next to `Dir`:

```cpp
export struct MapCoord {
    int col = 0;
    int row = 0;

    friend bool operator==(const MapCoord&, const MapCoord&) = default;
};
```

Defaulted `operator==` (and the implicit `!=`) is enough — no ordering needed. No hashing yet; add when a use case appears.

### 2. `engine.random` module (new)

New files:
- `src/engine/random.ixx`
- `src/engine/random.cpp`

```cpp
// random.ixx
export module engine.random;

export class Random {
public:
    static Random& instance();

    // Inclusive on both ends.
    int  int_in(int lo, int hi);
    float float_in(float lo, float hi);

    void seed(unsigned s);
private:
    Random();
    // std::mt19937 engine_; — kept in .cpp to avoid leaking <random> into the interface
};

export int   random_int  (int lo, int hi);
export float random_float(float lo, float hi);
```

`random.cpp` holds the `mt19937` member (declared via PIMPL-style member in the `.cpp` after `module engine.random;` since we cannot put non-exported members across TU boundaries — instead just put `<random>` in the GMF of `random.ixx` and declare `std::mt19937 engine_;` as a private member there; the include stays in the GMF, not in the export). Simpler approach: `<random>` in `random.ixx` GMF; member is private; no PIMPL needed.

Constructor seeds from `std::random_device{}()` once. `seed(s)` is for deterministic testing/repro.

Add `src/engine/random.{ixx,cpp}` to `CMakeLists.txt` (sources + module file set), placed before `engine.renderer` ordering-wise — alphabetical group fine.

### 3. `Map` additions

`src/game/map.ixx`:

```cpp
// New overloads — coexist with existing int-pair ones for now.
[[nodiscard]] Tile tile_at_index(MapCoord c) const;
[[nodiscard]] bool is_wall_at  (MapCoord c) const;
void              clear_tile   (MapCoord c);

[[nodiscard]] MapCoord pick_random_walkable() const;
```

`map.cpp`:
- Each `MapCoord` overload is a one-line forward to the existing int-pair version (preserving the current arg orders inside Map). Keeps semantics identical, no callers forced to switch in this PR.
- `pick_random_walkable()` — rejection sampling: draw `col ∈ [0, MAP_COLS)` and `row ∈ [0, MAP_ROWS)` via `random_int`; loop while `at(row, col) == Tile::Wall`. Map is ~32% walls so the expected iteration count is ~1.5; cap loop iterations at e.g. 1024 as a defence against a hypothetical all-walls map and `log_warn` if hit.

The existing int-pair signatures are **kept** — `Pacman` and `Stage` still call them with `int` locals (`stage.cpp:48-58`, `pacman.cpp:can_move`). No source churn outside the Ghost/Map/debug surface.

### 4. `Ghost` refactor

`src/game/ghost.ixx`:
- Drop `#include <utility>` from GMF; replace with whatever `MapCoord` brings via `game.types` (already imported).
- `std::pair<int,int> pick_random_target();` → `MapCoord pick_random_target();`
- `void move_toward(int col, int row, float dt);` → `void move_toward(MapCoord target, float dt);`
- `bool ghost_reached(int col, int row);` → `bool ghost_reached(MapCoord target);`

`src/game/ghost.cpp`:
- `move_to::target_` becomes `MapCoord target_`.
- `move_to` ctor takes `MapCoord target`.
- `update(float dt)` calls `ghost_.move_toward(target_, dt)` and `ghost_.ghost_reached(target_)`.
- `pick_random_target()` becomes `return map_->pick_random_walkable();` (uses the cached `map_` pointer set in `reset`).

### 5. Debug structs

`src/game/types.ixx`:

```cpp
export struct PacmanDebugState {
    MapCoord coord;
    int dir_x, dir_y;
    float speed;
    AABB  bounds;
};

export struct GhostDebugState {
    GhostId    id;
    MapCoord   coord;
    int        dir_x, dir_y;
    float      speed;
    AABB       bounds;
    GhostState state;
    MapCoord   target;          // {-1,-1} when not applicable
};
```

(Drops `int col, row` / `int target_col, target_row` field pairs.)

### 6. Consumer updates

- `Pacman::debug_state()` (`pacman.cpp`) — populate `.coord = { col_, row_ }`.
- New `Pacman::current_coord()` inline accessor in `pacman.ixx` returning `{ col_, row_ }`. Existing `current_col()`/`current_row()` stay (still used in `stage.cpp:49-50`).
- `DebugView::draw_pacman_section` / `draw_ghost_section` / `draw_map_section` in `src/game/debug.cpp` — change `pacman.col`/`.row` → `pacman.coord.col`/`.coord.row`, same for `ghost.target_col`/`.target_row` → `ghost.target.col`/`.target.row`. The `target_col >= 0` guard becomes `ghost.target.col >= 0`.
- Any ghost code that produces `GhostDebugState` (none yet in tree besides type def) will populate `.coord` and `.target`.

### 7. Files modified

- `src/game/types.ixx` — add `MapCoord`; rewrite debug structs.
- `src/engine/random.ixx`, `src/engine/random.cpp` — **new**.
- `src/game/map.ixx`, `src/game/map.cpp` — new overloads + `pick_random_walkable`.
- `src/game/ghost.ixx`, `src/game/ghost.cpp` — drop pair, use MapCoord.
- `src/game/pacman.ixx`, `src/game/pacman.cpp` — add `current_coord()`; populate `debug_state().coord`.
- `src/game/debug.cpp` — field-access rename in three sections.
- `CMakeLists.txt` — add `engine/random.{ixx,cpp}` to sources and module file set.

No changes required in: `stage.cpp` (still uses ints), `main.cpp`, `engine.renderer`, `engine.input`, `engine.log`, `game.console`, `game.config`, `game.scheduler`.

## Verification

1. `/build` (clang-ninja-debug) — clean compile; `static_assert(GameEntity<Pacman>)` still holds.
2. Launch the game. Debug panel shows Pac-Man coord and (after Ghost wires up properly later) ghost coord/target without format regressions.
3. Console log shows repeated `pick_random_target called` entries — verify the returned coords are inside the map and never land on a Wall tile by sampling a few via the existing trace logging (extend the existing `log_trace` in `pick_random_target` to include the picked col/row).
4. Determinism check: temporarily call `Random::instance().seed(42)` from `main.cpp` before `stage.reset()`; run twice; confirm the same sequence of `pick_random_target` log entries.
5. MSVC preset build also clean (project's stated quality gate).
