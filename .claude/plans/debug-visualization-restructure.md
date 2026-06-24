# Restructure Debug Visualization

## Context

The `Debug` window (`src/game/debug.cpp`) is a single `ImGui::Begin("Debug")` with
everything stacked as collapsing headers: Pac-Man, four separate per-ghost headers, Tweaks,
and the minimap. Two problems: (1) the minimap — a spatial view — is buried under textual
readouts and shares scroll/space with them; (2) four per-ghost headers (~7 lines each) make
cross-ghost comparison (who's chasing, who's still in the house, target tiles) require
expanding and eyeballing four blocks. There is also no read-only view of the orchestration
state (`GameState`) that drives house-release timing, even though `Stage` already owns it.

Goal: a more rational layout — minimap as its own window, ghosts as a single comparison
table, and a compact read-only GameState panel for the orchestration fields.

Delegation tier: **T3** (AI drafts, author owns). This is ImGui/debug tooling, not core
C++20 or entity-system work. `DebugView` is explicitly the *what*; `Renderer` owns the
*when* (working_agreement.md), and that boundary is untouched here.

## Target layout

**Window `Debug`** (textual state, top-to-bottom):
1. Pac-Man section — unchanged
2. **Ghosts table** — one row per ghost (replaces the 4 per-ghost headers)
3. **GameState section** — new, orchestration fields only, read-only
4. Tweaks section — unchanged

**Window `Minimap`** (new top-level window): the current minimap render, lifted out verbatim.

## Changes

### 1. `src/game/types.ixx` — minimap layout constants
Add `LAYOUT_MINIMAP_*` near the other `LAYOUT_*` constants (lines 22–33). Recommended
placement: a third column to the right of the Debug window, full game height —
```
LAYOUT_MINIMAP_X = LAYOUT_DEBUG_X + DEBUG_PANEL_W + 40
LAYOUT_MINIMAP_Y = 10
LAYOUT_MINIMAP_W = MAP_COLS * (12 + 2) + ~20   // minimap is 392 px wide + padding
LAYOUT_MINIMAP_H = WINDOW_H
```
This avoids cramming the minimap into the right column under a shortened Debug window.
Optionally widen `LAYOUT_CONSOLE_W` / `LAYOUT_WINDOW_W` to span the new column — tunable,
Ludo's call on final coordinates.

### 2. `src/game/debug.ixx` — interface
- Change `draw(...)` to thread `GameState` through:
  `void draw(const Map& map, const PacmanDebugState& pacman, std::span<const GhostDebugState> ghosts, const GameState& game_state, GameConfig& config);`
- Replace private decls:
  - `draw_ghost_section(const GhostDebugState&)` → `draw_ghosts_table(std::span<const GhostDebugState>)`
  - `draw_map_section(...)` → `draw_minimap_window(const Map&, const PacmanDebugState&, std::span<const GhostDebugState>)`
  - add `draw_gamestate_section(const GameState&)`
- `game.types` is already imported, so `GameState` is in scope.

### 3. `src/game/debug.cpp` — implementation
- **`draw()`**: keep `Begin("Debug")` containing pacman → ghosts table → gamestate → tweaks.
  After `End()`, call `draw_minimap_window(...)` (its own top-level window).
- **`draw_ghosts_table()`**: `ImGui::BeginTable("ghosts", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)`.
  Columns: `Ghost | pos | dir | speed | state | target`. Per row: ghost name tinted with
  `ghost.color` (`ImGui::TextColored`), `col,row`, `dir_x,dir_y`, `%.1f` speed, state string,
  and target as `col,row` or `"-"` when `target.col < 0`. **AABB dropped** (visible on the
  minimap bounds rect). Wrap the whole table in a `CollapsingHeader("Ghosts", DefaultOpen)`
  to match the section idiom.
- **`draw_gamestate_section()`**: `CollapsingHeader("Game State", DefaultOpen)`, read-only
  `ImGui::Text` for the **orchestration-only** fields:
  `dots_eaten`, `dot_timer` (`%.2f`), `next_force_release` (ghost name or `<none>` via the
  optional), `next_ghost_release_index`. Skip `pacman_tile`/`pacman_dir`/`blinky_tile` —
  already shown in the Pac-Man section and ghost table.
- **`draw_minimap_window()`**: lift the existing `draw_map_section` body unchanged (tiles,
  Pac-Man bounds rect, ghost circles, target X markers, path lines, the trailing
  `ImGui::Dummy`), wrapped in its own `SetNextWindowPos/Size(LAYOUT_MINIMAP_*)` +
  `Begin("Minimap")` / `End()`. No longer a `CollapsingHeader`.
- **De-dup ghost names**: the `{"Blinky","Pinky","Inky","Clyde"}` array now needs reuse in
  both the table and the GameState `next_force_release` label. Add a file-local
  `static const char* ghost_name(GhostId)` helper in debug.cpp and use it in both places.
  Keep it local — no new module-level abstraction.

### 4. `src/game/stage.cpp` — pass GameState
Line 103: `debug_.draw(map_, pacman_.debug_state(), ghosts, game_state_, config_);`
`game_state_` is already populated each frame (stage.cpp:73–76) before the draw call.

## Verification
1. Build: `/build` (clang-ninja-debug) — must be clean under `/W4 /permissive-`.
2. Run the app; toggle the debug view on.
3. Confirm the **Debug** window shows, in order: Pac-Man, a **Ghosts table** (4 rows, names
   color-tinted, states/targets readable at a glance), a **Game State** panel
   (dots_eaten / dot_timer / next_force_release / next_ghost_release_index updating live),
   and Tweaks.
4. Confirm the **Minimap** is a separate window rendering tiles, Pac-Man, ghost dots, target
   markers, and path lines exactly as before.
5. Eat dots and watch `dots_eaten` climb and `next_force_release` advance as ghosts leave the
   house — sanity-checks the GameState wiring.
