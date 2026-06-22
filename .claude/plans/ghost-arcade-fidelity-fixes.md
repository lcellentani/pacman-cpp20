# Ghost AI — Arcade-Fidelity Fixes (Items 1, 2, 3)

## Context

A review of `src/game/ghost.cpp` found three places where the current scatter/chase
implementation departs from the original arcade Pac-Man ghost logic. All three affect how
faithfully the ghosts behave compared to the 1980 ROM:

1. **Pinky's "facing up" overflow bug is missing.** Inky's targeting (`calculate_inky_target`)
   deliberately reproduces the famous arcade overflow — when Pac-Man faces up, the "N tiles
   ahead" lookup also shifts left by N. Pinky's target (`pacman_tile + 4*dir`) does **not**
   reproduce it. In the real game both ghosts share the same buggy "ahead" lookup, so this is
   an inconsistency, not a design choice.

2. **Tie-breaking is random instead of deterministic.** `move_toward_greedy` collects all
   directions tied for minimum distance and picks one with `random_int`. The arcade AI is
   fully deterministic: ties break by fixed priority **Up > Left > Down > Right**. This is the
   single biggest behavioral departure — it makes ghost paths non-reproducible.

3. **No forced reversal on scatter↔chase mode switch.** In the arcade, every ghost reverses
   direction the instant the global mode flips. Here, switching `wait_for` phases only changes
   the target; the ghost reverses only incidentally when greedy selection happens to pick it.

**Intended outcome:** ghosts that move deterministically and reproduce the canonical arcade
targeting/reversal rules. Per project preference, new pure helpers are written as `constexpr`
free functions and existing constexpr utilities are reused.

## Files to modify

- `src/game/types.ixx` — reorder `cardinal_dirs()` to encode arcade tie-break priority (Item 2)
- `src/game/ghost.cpp` — targeting helpers, greedy tie-break, forced reversal, phase tracking
- `src/game/ghost.ixx` — new members + adjusted private declarations

`engine.random` becomes unused in this module after Item 2 (see notes).

### Impact analysis: reordering `cardinal_dirs()`

There are exactly two consumers of `cardinal_dirs()`:

- **`Map::find_path` (map.cpp:158)** — BFS neighbor expansion. BFS shortest-path correctness is
  **independent of neighbor visitation order**; order only changes *which* of several equal-length
  paths is reconstructed via `parent[]`. Moreover `find_path` currently has **no callers**
  (verified — defined in map.cpp/map.ixx, never invoked), so the change has zero observable
  effect there today.
- **`Ghost::move_toward_greedy` (ghost.cpp:368)** — the function Item 2 rewrites, where priority
  ordering is precisely what we want.

Conclusion: reordering is safe. Item 2 reuses `cardinal_dirs()` rather than introducing a
parallel `priority` array, keeping a single source of truth for direction order.

---

## Item 1 — Pinky/Inky shared "ahead" lookup with the up-overflow bug

Add a single `constexpr` free function in `ghost.cpp` (file scope, near `distance_squared`)
that both Pinky and Inky reuse. It reproduces the arcade overflow: facing up shifts both row
and column by `-n`.

```cpp
// Arcade "n tiles ahead of Pac-Man" lookup. Reproduces the original ROM
// overflow bug: when Pac-Man faces up, the column is also shifted by -n.
constexpr MapCoord arcade_ahead(MapCoord tile, Dir dir, int n) {
    int col = tile.col + dir.x * n;
    int row = tile.row + dir.y * n;
    if (dir.x == 0 && dir.y == -1) col -= n;   // up: overflow bug
    return { col, row };
}
```

- **Pinky** (`pick_chase_target_for_ghost`, Pinky branch): replace the manual
  `pacman_col + 4*dir_x` math with `return arcade_ahead(game_state_->pacman_tile, game_state_->pacman_dir, 4);`
- **Inky**: rewrite `calculate_inky_target` as a `constexpr` free function that reuses
  `arcade_ahead(..., 2)` for the pivot, then doubles the Blinky→pivot vector:

```cpp
constexpr MapCoord inky_target(MapCoord pac, Dir pac_dir, MapCoord blinky) {
    const MapCoord pivot = arcade_ahead(pac, pac_dir, 2);
    const int vx = pivot.col - blinky.col;
    const int vy = pivot.row - blinky.row;
    return { blinky.col + 2 * vx, blinky.row + 2 * vy };
}
```

This removes the duplicated offset logic in the current `calculate_inky_target`
(ghost.cpp:238-259) and makes the up-bug live in exactly one place.

---

## Item 2 — Deterministic tie-break (Up > Left > Down > Right)

### Step A — reorder `cardinal_dirs()` (`src/game/types.ixx:50-53`)

Change the static array so its order encodes the arcade tie-break priority, and document it:

```cpp
// Order encodes the arcade ghost tie-break priority: Up > Left > Down > Right.
// Consumers that break ties by "first acceptable direction" rely on this order.
export [[nodiscard]] inline std::span<const Dir> cardinal_dirs() {
    static constexpr std::array<Dir, 4> dirs = { { { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, 0 } } };
    return dirs;
}
```

(Was `{ {1,0}, {-1,0}, {0,1}, {0,-1} }`.) See the impact analysis above — `find_path` is the
only other consumer and is order-insensitive and uncalled.

### Step B — rewrite the decision block in `move_toward_greedy` (ghost.cpp:352-383)

Iterate `cardinal_dirs()` (now in priority order) and keep the **first** direction at the minimum
distance (strict `<`), so higher-priority directions win ties. Reuse the existing
`constexpr distance_squared` helper.

```cpp
if (offset_ == 0) {
    const Dir reverse_dir{ -current_dir_.x, -current_dir_.y };

    Dir best_dir{ 0, 0 };
    int min_dist = std::numeric_limits<int>::max();
    bool found = false;

    for (const Dir& dir : cardinal_dirs()) {   // priority order: Up > Left > Down > Right
        if (dir.x == reverse_dir.x && dir.y == reverse_dir.y) continue;
        if (!can_move(col_, row_, dir)) continue;
        const int dist = distance_squared({ col_ + dir.x, row_ + dir.y }, target);
        if (dist < min_dist) {        // strict <: first (highest priority) wins ties
            min_dist = dist;
            best_dir = dir;
            found = true;
        }
    }

    // Dead-end fallback: reverse only when no other walkable direction exists.
    if (!found && can_move(col_, row_, reverse_dir)) {
        best_dir = reverse_dir;
        found = true;
    }

    if (found) current_dir_ = best_dir;
}
```

This removes the `best_dirs`/`best_count`/`consider` machinery and the `random_int` call.

**Note:** after this change `import engine.random` is unused in `ghost.cpp`. **Keep the import** —
frightened mode (Phase 3 remainder) will reuse `random_int`. (Unused module imports do not
trigger `/W4` warnings.) Mention in the commit message so it isn't mistaken for dead code.

---

## Item 3 — Forced reversal on scatter↔chase switch + real phase tracking

The coroutine `behavior()` is the one place that knows when the mode changes, so drive the
reversal from there. This also fixes the hard-coded `GhostState::Chase` in `debug_state()`
(an existing open item) as a natural side effect.

### New members (`ghost.ixx`)

```cpp
GhostState  state_ = GhostState::Scatter;  // actual current phase
bool        phase_started_ = false;        // suppress reversal on the very first phase
bool        reverse_pending_ = false;      // set on mode switch, consumed at next tile center
```

### New private helper (`ghost.ixx` + `ghost.cpp`)

```cpp
void Ghost::enter_phase(GhostState s) {
    if (phase_started_ && s != state_)
        reverse_pending_ = true;   // arcade: reverse on every mode change after the first
    state_ = s;
    phase_started_ = true;
}
```

### `behavior()` changes (ghost.cpp:174-200)

At the start of each scatter phase call `enter_phase(GhostState::Scatter)`; at the start of
each chase phase call `enter_phase(GhostState::Chase)`. Keep the existing `target_ = ...`
assignment alongside. The initial scatter (first loop iteration) sets `phase_started_` without
flagging a reversal, matching a ghost that just left the house.

### Consume the flag in `move_toward_greedy`

At the top of the `offset_ == 0` block, before the greedy selection, honor a pending reversal
and skip normal selection for that tick:

```cpp
if (offset_ == 0) {
    if (reverse_pending_) {
        reverse_pending_ = false;
        const Dir rev{ -current_dir_.x, -current_dir_.y };
        if ((rev.x != 0 || rev.y != 0) && can_move(col_, row_, rev)) {
            current_dir_ = rev;
        }
        // fall through to advance offset with the (possibly reversed) direction
    } else {
        // ... deterministic greedy selection from Item 2 ...
    }
}
```

### `debug_state()` (ghost.cpp:159-161)

Replace the hard-coded `GhostState::Chase` with `state_`.

**Reversal-timing note:** the flag is consumed at the next tile center (`offset_ == 0`), so a
ghost mid-tile at the moment of the switch finishes its current tile before reversing — at most
one tile of latency. This is simpler than Pac-Man's mid-tile flip and visually indistinguishable
at gameplay speed; documented here as a deliberate, minor simplification.

---

## `ghost.ixx` declaration changes (summary)

- Add the three members above (`state_`, `phase_started_`, `reverse_pending_`).
- Add `void enter_phase(GhostState s);` private declaration.
- Convert `pick_scatter_target_for_ghost` to a `constexpr` free function `scatter_target_for(GhostId)`
  in `ghost.cpp` (it is pure); remove its member declaration. Update Clyde's chase branch to call it.
- Remove the member declaration for the old `calculate_inky_target` if any (currently a free
  function — just rewrite it). `arcade_ahead`, `inky_target`, `scatter_target_for` are all
  file-scope `constexpr` free functions in `ghost.cpp`.

---

## Verification

No automated tests exist; the gate is a clean `/W4 /permissive-` build plus visual inspection
via the debug overlay.

1. **Build:** `cmake --build --preset clang-ninja-debug` (the `build-on-write.sh` hook also runs
   on each save). Must compile clean — pay attention to the new `constexpr` functions and the
   `<array>`/`<limits>` includes already present in `ghost.cpp`.
2. **Run** the app and press `D` to open the debug panel. For each ghost the panel shows
   `state`, `target`, and `path`.
3. **Item 1:** steer Pac-Man so he faces **up** at an intersection. Confirm Pinky's target tile
   sits up-and-to-the-left of Pac-Man (not straight up), and that Inky's target reflects the
   same shifted pivot.
4. **Item 2:** ghost movement is now deterministic — from a fixed start the same path repeats
   every run. Confirm no jitter/random direction flips at junctions.
5. **Item 3:** watch the debug `state` field flip Scatter↔Chase on the phase timer; confirm each
   ghost visibly reverses direction at (or one tile after) the switch, and that the `state`
   field is no longer stuck on `Chase`.
