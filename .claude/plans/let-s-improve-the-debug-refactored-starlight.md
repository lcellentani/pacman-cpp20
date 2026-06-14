# Plan: Improve Ghost Debug Visualization in Minimap

## Context

The debug minimap currently draws each ghost as an outlined rectangle (`AddRect`) matching
its AABB bounds. This is problematic because:
- A 1 px outline in ghost color is nearly invisible against white pellet tiles
- The outline shape gives no visual hierarchy — it looks the same as any other border
- Target tiles are drawn as fully solid filled squares (`AddRectFilled`) in the ghost
  color, which covers the underlying tile type and can be confused with map tiles
  (especially Pinky/Inky whose colors resemble Door/Wall tiles)
- The target draw has no guard for the invalid `{-1, -1}` value used in Frightened/Dead
  states, so a stray tile can appear in the top-left corner of the map

## Proposed Changes — single file: `src/game/debug.cpp`

### 1. Ghost body (lines 124–134): outline rect → filled circle with dark ring

Replace `AddRect(tl, br, color)` with a filled circle centered on the ghost AABB,
plus a thin dark `AddCircle` outline so the ghost stays legible against any tile color
(white pellets, cyan Inky, pink Pinky, etc.).

```cpp
// center derived from AABB (sub-tile precision preserved)
float cx = origin.x + (ghost.bounds.x + ghost.bounds.width  * 0.5f) * SCALE;
float cy = origin.y + (ghost.bounds.y + ghost.bounds.height * 0.5f) * SCALE;
float r  = MINI_TILE * 0.45f;            // ~5.4 px — fills tile without clipping
draw_list->AddCircleFilled({cx, cy}, r, color);
draw_list->AddCircle({cx, cy}, r, IM_COL32(0, 0, 0, 180), 0, 1.0f);
```

`AddCircleFilled` / `AddCircle` are standard ImDrawList calls — no new renderer
primitives needed.

### 2. Target tile (lines 136–143): solid fill → outlined rect + X crosshair

Replace `AddRectFilled(tl, br, color)` with an outlined rect and two diagonal lines.
This marks the target cell without obscuring which tile type lives there, and reads
clearly as a "marker" rather than a map element.

Also add the validity guard (`ghost.target.col >= 0`) that is already applied in the
text section (line 43) but is currently missing from the minimap draw.

```cpp
if (ghost.target.col >= 0) {
    float x = ghost.target.col * (MINI_TILE + PADDING);
    float y = ghost.target.row * (MINI_TILE + PADDING);
    ImVec2 tl { origin.x + x,             origin.y + y };
    ImVec2 br { tl.x + MINI_TILE,         tl.y + MINI_TILE };
    ImVec2 tr { tl.x + MINI_TILE,         tl.y };
    ImVec2 bl { tl.x,                     tl.y + MINI_TILE };

    const ImU32 faded = IM_COL32(ghost.color.r, ghost.color.g, ghost.color.b, 180);
    draw_list->AddRect(tl, br, color, 0.0f, 0, 1.5f);   // outline border
    draw_list->AddLine(tl, br, faded, 1.0f);              // top-left → bottom-right
    draw_list->AddLine(tr, bl, faded, 1.0f);              // top-right → bottom-left
}
```

### 3. Path lines (lines 145–158): no change

Semi-transparent polyline already reads well — leave it as-is.

## Files modified

| File | Change |
|---|---|
| `src/game/debug.cpp` | Two rendering blocks changed (ghost body, target tile) |

No interface changes — `debug.ixx`, `types.ixx`, ghost modules are untouched.

## Rendering order within each ghost (unchanged)

1. Target tile marker (drawn first, underneath)
2. Ghost body circle (drawn on top of target if they coincide)
3. Path polyline (drawn last, overlaps everything)

## Verification

Build with `/build` and launch the game (`r` to run). Toggle debug with `D`.
Check in minimap:
- Each ghost appears as a colored filled circle clearly distinct from white pellets
- Moving Blinky (red) through a pellet-filled corridor still readable
- Target tile shows as an outlined X-marker, not a solid block
- In Frightened/Dead state the target marker disappears (guard fires)
- Path polylines still connect correctly from ghost tile through waypoints
