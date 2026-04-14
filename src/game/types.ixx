export module game.types;

import engine.types;

export constexpr int MAP_COLS = 25;
export constexpr int MAP_ROWS = 31;
export constexpr int TILE_SIZE = 24;   // 28*24=672 — same window width as before
export constexpr int WINDOW_W = MAP_COLS * TILE_SIZE;   // 672
export constexpr int WINDOW_H = MAP_ROWS * TILE_SIZE;   // 744
export constexpr int DEBUG_PANEL_W = 380;

export struct PacmanDebugState {
    float x, y;
    float dx, dy;
    float speed;
    AABB  bounds;
};