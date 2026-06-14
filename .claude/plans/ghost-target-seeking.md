# Ghost Target Seeking — Plan (House Exit + Greedy Seek)

## Context

The first draft of this plan described a greedy direction-selection algorithm:
at each tile center, evaluate all four cardinal directions via `can_move`, score
them by squared distance to `target_`, pick the minimum. The algorithm is
correct **only for ghosts already on the open map**. The gap it did not address:

- Three of the four ghosts start **inside the ghost house** at row 14
  (Pinky 13,14 / Inky 11,14 / Clyde 15,14).
- The only opening to the corridor above is at **Door tiles (13,12) and (14,12)**.
- `Map::is_wall_at` returns `true` for `Tile::Door` (`src/game/map.cpp:103`), so
  greedy `can_move` rejects the exit edge. Pinky, Inky and Clyde would be
  permanently stuck oscillating inside the house.

This revision adds a **scripted exit coroutine** that runs before the greedy
seek loop. It is consistent with Phase 3's coroutine ethos — the exit is
expressed as a sequence of `co_await`-ed steps rather than as a hidden mode
flag inside `move_toward`. After clearing the door, every ghost falls into the
same `wander()` loop Blinky already uses, with a stronger greedy seek that also
honours the arcade no-reverse rule.

Confirmed design choices for this revision:
- **Exit strategy:** scripted exit coroutine using a path-walking awaiter that
  ignores wall/door checks (not a `can_move` flag and not a `GhostMode` enum).
- **First post-exit target:** `pick_random_target()`, same as Blinky's current
  `wander()` body. Scatter corners are a later step.
- **No-reverse rule:** enforced now — at each tile center, exclude the
  direction opposite to `current_dir_` from the candidate set.

## Geometry (verified)

```
row 11:  . . . . . . . . . . . . . . . . . . . . . . . . . . . .   <- Blinky start col 13, open corridor
row 12:  W W W W W W W W W W W W D D W W W W W W W W W W W W W W   <- Door tiles at (13,12) and (14,12)
row 13:  . . . . . . . . . . . G G G G G G . . . . . . . . . . .   <- House interior
row 14:  . . . . . . . . . . . G G G G G G . . . . . . . . . . .   <- Inky 11, Pinky 13, Clyde 15
row 15:  . . . . . . . . . . . G G G G G G . . . . . . . . . . .   <- House interior
```

Exit path per ghost:
- **Blinky (13,11)** — already outside, skip the exit step.
- **Pinky (13,14)** — straight up through (13,13) → (13,12) → (13,11).
- **Inky (11,14)** — right to (12,14) → (13,14), then up through the door to (13,11).
- **Clyde (15,14)** — left to (14,14) → (13,14), then up through the door to (13,11).

## Design

Two awaiters drive the ghost. Both live as private structs inside
`src/game/ghost.cpp` (same TU as the existing `move_to`).

### 1. `move_to` — greedy seek (extends the existing struct)

Each `update(dt)` call invokes the new method `Ghost::move_toward_greedy(target, dt)`,
which is the existing `move_toward` body with the decision point at
`offset_ == 0` filled in. Resumption uses `Ghost::ghost_reached(target)`, no
longer a stub.

### 2. `walk_path` — scripted, wall-blind (new struct)

Holds a sequence of waypoints (`std::span<const MapCoord>`) ending at the
post-exit tile (13,11). On each `update(dt)` it advances the ghost one pixel at
a time along the leg's direction **without consulting `can_move`** — this is
what lets it cross the door tile. When the ghost's tile coordinate matches the
next waypoint, it advances to the next leg; when the final waypoint is reached,
it resumes the coroutine.

The scripted advance uses a new helper `Ghost::step_unchecked(Dir dir, float dt)`
that mirrors `move_toward_greedy` minus the direction picker and minus the
wall check — same accumulator, same offset arithmetic.

### 3. `leave_house()` coroutine

```cpp
Task Ghost::leave_house(Scheduler& s) {
    // path_for(id_) returns a per-ghost waypoint sequence ending at (13,11)
    //   Pinky:  { (13,11) }
    //   Inky:   { (13,14), (13,11) }   // right-align, then up through door
    //   Clyde:  { (13,14), (13,11) }   // left-align, then up through door
    co_await walk_path(*this, s, path_for(id_));
}
```

`path_for(GhostId)` returns a static `std::span<const MapCoord>` (or a small
`std::array` per id). The awaiter infers direction between consecutive waypoints;
the path constraint (one axis at a time, integer tile steps) keeps this trivial.

### 4. Behavior dispatcher

Replace the current `Ghost::wander()` with a single `Ghost::behavior(Scheduler&)`
that runs the exit first and then loops on random targets:

```cpp
Task Ghost::behavior(Scheduler& s) {
    if (id_ != GhostId::Blinky) {
        co_await leave_house(s);
    }
    while (true) {
        target_ = pick_random_target();
        co_await move_to(*this, s, target_);
    }
}
```

`Ghost::reset()` stores `behavior_ = behavior(s)` and resumes it once.

### 5. Greedy direction picker

Extract a private helper:

```cpp
Dir Ghost::pick_greedy_direction(MapCoord target) const;
```

Inputs: current `col_`, `row_`, `current_dir_`, `target`.
Algorithm:
1. Candidates = `{ {1,0}, {-1,0}, {0,1}, {0,-1} }`.
2. Drop any direction `d` where `can_move(col_, row_, d)` is false.
3. Drop the direction opposite to `current_dir_` (skip this filter only when
   `current_dir_ == {0,0}` — the initial state has no "reverse" to forbid).
4. Score each survivor by `(col_ + d.x - target.col)^2 + (row_ + d.y - target.row)^2`.
5. Return the minimum-scoring direction; on ties pick uniformly at random
   (use the same RNG already in use by `Map::pick_random_walkable`, or a local
   `std::mt19937` seeded once — to be decided during implementation).
6. If no candidate survives (dead end after reverse-exclusion), return the
   reversal anyway; only return `{0,0}` if the ghost is literally walled in
   (should not happen on this map).

Add a small `is_opposite(Dir a, Dir b)` free function in `game.types` (or a
private static in `ghost.cpp`) — guard against the zero direction the same way
`Pacman` already does.

### 6. `ghost_reached`

```cpp
bool Ghost::ghost_reached(MapCoord target) {
    return offset_ == 0 && col_ == target.col && row_ == target.row;
}
```

Called from `move_to::update` after `move_toward_greedy`; the awaiter resumes
the coroutine on first true.

## Files modified

- `src/game/ghost.ixx` — declare `pick_greedy_direction`, `move_toward_greedy`,
  `step_unchecked`, `leave_house`, `behavior`; remove `wander` and the current
  stub form of `move_toward`; `pick_random_target` keeps its signature;
  befriend `walk_path`.
- `src/game/ghost.cpp` —
  - Fill in `move_toward_greedy` (existing `move_toward` body + direction picker
    at `offset_ == 0`).
  - Add `step_unchecked` (offset-advance only, no wall check).
  - Implement `pick_greedy_direction` with no-reverse and random tiebreak.
  - Implement `ghost_reached` (no longer a stub).
  - Add `walk_path` struct (mirrors `move_to`'s shape).
  - Add `path_for(GhostId)` table and `leave_house()` coroutine.
  - Replace `wander()` with `behavior()`; update `reset()` to assign it.
- No changes to `src/game/map.cpp` — the Door tile keeps its
  "solid for normal collision" semantics; only the scripted awaiter bypasses it.
- No changes to `engine.*` or `game.scheduler`.

## What does not change

- `Map::is_wall_at` semantics for `Tile::Door`.
- `pick_random_walkable` — already excludes Door tiles, so targets always lie
  outside the house.
- `Scheduler` / `Updatable` concept — both awaiters already satisfy
  `Updatable<T>` via `void update(float dt)`.
- The `static_assert(GameEntity<Ghost>)` is a separate open item from CLAUDE.md
  and stays out of scope here.

## Verification

1. **Build:** `cmake --build --preset clang-ninja-debug` — must compile clean
   under `/W4 /permissive-` (and the equivalent Clang strict flags).
2. **Run the game and observe with the debug panel (D key):**
   - Blinky immediately starts seeking a random target; minimap shows him
     pursuing a colored square.
   - Pinky walks straight up from (13,14), crossing the door tile, then begins
     random seeking from row 11 — confirm she does pass through (13,12).
   - Inky steps right to align with column 13, then exits upward.
   - Clyde steps left to align with column 13, then exits upward.
   - Once outside, each ghost's debug section shows changing `target`/`pos`/`vel`
     and the AABB rect on the minimap tracks toward the target square.
3. **No-reverse check:** sit at a 4-way junction and verify that on greedy
   re-evaluation the ghost never instantaneously U-turns (it may U-turn only
   at dead ends).
4. **No-stuck check:** leave the game running for 30+ seconds; verify no ghost
   parks at a single tile (would indicate `current_dir_` collapsed to `{0,0}`
   on the open map).
5. **Tiebreak determinism is fine to be non-deterministic:** repeat runs may
   show different paths for the same start — that is the intended behaviour of
   the random tiebreak.

No automated tests — the project has no test framework, and visual validation
via the debug panel is the established Phase 2/3 quality gate.

## Technical constraints (carried over)

- No virtual functions; stick to Concepts and direct class methods.
- Maintain tile-based movement with a pixel-level offset accumulator.
- `current_dir_` is never `{0,0}` once movement is underway, except when the
  ghost is literally walled in (should not happen on this map).
