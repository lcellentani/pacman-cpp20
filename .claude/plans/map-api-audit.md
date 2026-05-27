# Map API Audit — `src/game/map.ixx`

## Context

Reviewing the public API of `game.map` for inconsistencies before Phase 3 (ghost AI),
which will introduce new callers to every Map query method.

---

## Findings

### 1. Parameter order split across the grid-coord API (primary issue)

Two naming conventions coexist in the same class:

| Method | Signature | Order |
|---|---|---|
| `tile_at_index` | `(int row, int col)` | **row-first** |
| `clear_tile`    | `(int col, int row)` | **col-first** |
| `is_wall_at`    | `(int col, int row)` | **col-first** |

`tile_at_index` is the odd one out. The private `at()` helper is `at(row, col)` (row-major
storage order), but that is an implementation detail — the public API should pick one
convention and stick to it.

**Are any callers currently passing args in the wrong order?**  
No — all call sites match their respective declarations:
- `stage.cpp` calls `tile_at_index(pac_row, pac_col)` and `clear_tile(pac_col, pac_row)`
- `debug.cpp` calls `tile_at_index(r, c)` (r = row, c = col)
- `pacman.cpp` calls `is_wall_at(col + dir.x, row + dir.y)`

So the inconsistency is latent (no bug today), but it is a trap for Phase 3 ghost callers.

### 2. `[[nodiscard]]` applied inconsistently

`tile_at_index` carries `[[nodiscard]]`; `tile_at` (the pixel-coord overload) does not.
Both return a `Tile` value — if one should be nodiscard, so should the other.

### 3. `is_wall` / `tile_at` have no bounds annotation

`tile_at` and `is_wall` accept raw pixel floats and silently clamp out-of-bounds to `Tile::Wall`.
`tile_at_index` and `is_wall_at` do the same for grid coords. The behavior is correct and
consistent — just not documented in the header.

---

## Options to resolve issue #1

**Option A — Standardise on `(col, row)` (matches x/y spatial intuition)**  
Rename `tile_at_index(int row, int col)` → `tile_at_index(int col, int row)`.  
Update the one internal call inside `is_wall_at`.  
No call-site changes needed (current callers already pass col-first to `clear_tile`/`is_wall_at`;
the `tile_at_index` callers pass `(pac_row, pac_col)` / `(r, c)` by name so a swap would need
their args flipped too).

**Option B — Standardise on `(row, col)` (matches internal storage order)**  
Rename `clear_tile(int col, int row)` → `clear_tile(int row, int col)` and same for `is_wall_at`.  
Update `stage.cpp` and `pacman.cpp` call sites.

**Option C — Drop `tile_at_index` from the public API**  
Only `debug.cpp` uses it for the minimap. Replace it with `tile_at(float, float)` pixel
overload, or keep it but document it clearly and add `[[nodiscard]]` to `tile_at`.

---

## Recommended resolution (to discuss)

Option A (`col, row` everywhere) aligns with spatial/screen convention (x before y),
matches `clear_tile` and `is_wall_at` as they stand today, and requires the fewest
call-site edits. Fix `[[nodiscard]]` on `tile_at` as a companion cleanup.

---

## Files affected (if Option A chosen)

- `src/game/map.ixx` — swap param names in `tile_at_index` declaration
- `src/game/map.cpp` — swap param names + swap the `at(row, col)` arg order in `tile_at_index` body; swap in `is_wall_at` body too
- `src/game/stage.cpp` — swap args at the two `tile_at_index(pac_row, pac_col)` call sites → `(pac_col, pac_row)`
- `src/game/debug.cpp` — swap args at `tile_at_index(r, c)` → `(c, r)`

## Verification

Build with `/build` (clang-ninja-debug). No test suite — visual check: pellets still
disappear on collection, walls still block movement, minimap still renders correctly.
