export module game.types;

import engine.types;

export constexpr int WINDOW_W = 672;   // MAP_COLS * TILE_SIZE
export constexpr int WINDOW_H = 672;   // MAP_ROWS * TILE_SIZE
export constexpr int DEBUG_PANEL_W = 400;

export struct PacmanDebugState {
    float x, y;       // world position
    float dx, dy;     // current velocity
    float speed;      // max speed
    AABB  bounds;     // real AABB — forces getBounds() fix
};