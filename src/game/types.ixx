export module game.types;

import engine.types;

export constexpr int MAP_COLS = 28;
export constexpr int MAP_ROWS = 31;
export constexpr int TILE_SIZE = 24;
export constexpr int HALF_TILE_SIZE = TILE_SIZE / 2; // 12
export constexpr int WINDOW_W = MAP_COLS * TILE_SIZE;   // 672
export constexpr int WINDOW_H = MAP_ROWS * TILE_SIZE;   // 744
export constexpr int DEBUG_PANEL_W = 430;

export struct PacmanDebugState {
    int col, row;
    int dir_x, dir_y;
    float speed;
    AABB  bounds;
};

export struct Dir { int x, y; }; // values always in {-1, 0, 1}