module;
#include <array>
#include <string>
#include <string_view>

module game.map;

import engine.log;
import engine.random;

// Classic Pac-Man wall layout — 28 cols x 31 rows.
//
// Key:
//   W = Wall
//   . = Open (pellets and gameplay items layered on top separately)
//
// The layout is the canonical Namco arcade map, verified:
//   - All rows are exactly 28 characters.
//   - The map is left-right symmetric.
//   - Row 14 is the tunnel row — both ends are open (no bounding wall).
//   - Rows 13–15 form the ghost house corridor.
//
static constexpr std::array<std::string_view, MAP_ROWS> k_layout = { {
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWW",  // row  0  — top border
    "W............WW............W",  // row  1
    "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",  // row  2
    "WSWWWW.WWWWW.WW.WWWWW.WWWWSW",  // row  3
    "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",  // row  4
    "W..........................W",  // row  5
    "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",  // row  6
    "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",  // row  7
    "W......WW....WW....WW......W",  // row  8
    "WWWWWW.WWWWW.WW.WWWWW.WWWWWW",  // row  9
    "WWWWWW.WWWWW.WW.WWWWW.WWWWWW",  // row 10
    "WWWWWW.WW..........WW.WWWWWW",  // row 11
    "WWWWWW.WW.WWWDDWWW.WW.WWWWWW",  // row 12  — ghost house door
    "WWWWWW.WW.WGGGGGGW.WW.WWWWWW",  // row 13  — ghost house top corridor
    ".......WW.WGGGGGGW.WW.......",  // row 14  — tunnel row (open ends)
    "WWWWWW.WW.WGGGGGGW.WW.WWWWWW",  // row 15  — ghost house bottom corridor
    "WWWWWW.WW.WWWWWWWW.WW.WWWWWW",  // row 16
    "WWWWWW.WW..........WW.WWWWWW",  // row 17
    "WWWWWW.WW.WWWWWWWW.WW.WWWWWW",  // row 18
    "WWWWWW.WW.WWWWWWWW.WW.WWWWWW",  // row 19
    "W............WW............W",  // row 20
    "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",  // row 21
    "W.WWWW.WWWWW.WW.WWWWW.WWWW.W",  // row 22
    "WS..WW................WW..SW",  // row 23  — Pac-Man start area
    "WWW.WW.WW.WWWWWWWW.WW.WW.WWW",  // row 24
    "WWW.WW.WW.WWWWWWWW.WW.WW.WWW",  // row 25
    "W......WW....WW....WW......W",  // row 26
    "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",  // row 27
    "W.WWWW.WW.WWWWWWWW.WW.WWWW.W",  // row 28
    "W..........................W",  // row 29
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWW",  // row 30  — bottom border
} };

Map::Map() {
    map_.fill(Tile::Empty);
}

// W = wall, P = pellet, . = empty
void Map::reset() {
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            switch (k_layout[r][c]) {
            case 'W': at(r, c) = Tile::Wall;  break;
            case '.': at(r, c) = Tile::Pellet; break;
			case 'S': at(r, c) = Tile::SuperPellet; break;
            case 'D': at(r, c) = Tile::Door; break;
            default:  at(r, c) = Tile::Empty; break;
            }
        }
    }
}

Tile Map::tile_at(float px, float py) const {
    const int col = static_cast<int>(px) / TILE_SIZE;
    const int row = static_cast<int>(py) / TILE_SIZE;
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
        return Tile::Wall;  // out-of-bounds treated as solid
    return at(row, col);
}

Tile Map::tile_at_index(int row, int col) const {
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS)
        return Tile::Wall;
    return at(row, col);
}

void Map::clear_tile(int col, int row) {
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS)
        return; // out-of-bounds, do nothing
    at(row, col) = Tile::Empty;
}

bool Map::is_wall(float px, float py) const {
    const auto t = tile_at(px, py);
    return t == Tile::Wall || t == Tile::Door;
}

bool Map::is_wall_at(int col, int row) const {
    const auto t = tile_at_index(row, col);
    return t == Tile::Wall || t == Tile::Door;
}

Tile Map::tile_at_index(MapCoord c) const {
    return tile_at_index(c.row, c.col);
}

void Map::clear_tile(MapCoord c) {
    clear_tile(c.col, c.row);
}

bool Map::is_wall_at(MapCoord c) const {
    return is_wall_at(c.col, c.row);
}

MapCoord Map::pick_random_walkable() const {
    // Rejection sampling. Map is roughly 1/3 walls, so expected attempts ~1.5.
    constexpr int MAX_ATTEMPTS = 1024;
    for (int i = 0; i < MAX_ATTEMPTS; ++i) {
        const int col = random_int(0, MAP_COLS - 1);
        const int row = random_int(0, MAP_ROWS - 1);
        if (at(row, col) != Tile::Wall && at(row, col) != Tile::Door)
            return MapCoord{ col, row };
    }
    log_warn("pick_random_walkable: exhausted " + std::to_string(MAX_ATTEMPTS) + " attempts");
    return MapCoord{ 0, 0 };
}

void Map::draw(Renderer& renderer) const {
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            switch (at(r, c)) {
            case Tile::Wall:
                renderer.draw_rect(
                    { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE },
                    { 33, 33, 222, 255 }
                );
                break;
            case Tile::Pellet:
                renderer.draw_rect(
                    { c * TILE_SIZE + 10, r * TILE_SIZE + 10, 4, 4 },
                    { 255, 255, 255, 255 }
                );
                break;
            case Tile::SuperPellet:
                renderer.draw_circle(
                    c * TILE_SIZE + HALF_TILE_SIZE,
                    r * TILE_SIZE + HALF_TILE_SIZE,
                    7,
                    { 255, 255, 255, 255 });
                break;
            case Tile::Door:
                renderer.draw_rect(
                    { c * TILE_SIZE + 2, r * TILE_SIZE + TILE_SIZE / 2 - 2, TILE_SIZE - 4, 4 },
                    { 255, 182, 255, 255 });
                break;
            default: break;
            }
        }
    }
}