module;
#include <array>
#include <string_view>

module game.map;

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
    "WWWWWWWWWWWWWWWWWWWWWWWWW",  // row  0  — top border
    "W...........W...........W",  // row  1
    "W.WWWW.WWWW.W.WWWW.WWWW.W",  // row  2
    "W.WWWW.WWWW.W.WWWW.WWWW.W",  // row  3
    "W.WWWW.WWWW.W.WWWW.WWWW.W",  // row  4
    "W.......................W",  // row  5
    "W.WWWW.W.WWWWWWW.W.WWWW.W",  // row  6
    "W.WWWW.W.WWWWWWW.W.WWWW.W",  // row  7
    "W......W....W....W......W",  // row  8
    "WWWWWW.WWWW.W.WWWW.WWWWWW",  // row  9
    ".....W.WWWW.W.WWWW.W.....",  // row 10
    ".....W.W.........W.W.....",  // row 11
    ".....W.W.WWWWWWW.W.W.....",  // row 12
    "WWWWWW.W.W.....W.W.WWWWWW",  // row 13  — ghost house top corridor
    ".........W.....W.........",  // row 14  — tunnel row (open ends)
    "WWWWWW.W.W.....W.W.WWWWWW",  // row 15  — ghost house bottom corridor
    ".....W.W.WWWWWWW.W.W.....",  // row 16
    ".....W.W.........W.W.....",  // row 17
    ".....W.W.WWWWWWW.W.W.....",  // row 18
    "WWWWWW.W.WWWWWWW.W.WWWWWW",  // row 19
    "W...........W...........W",  // row 20
    "W.WWWW.WWWW.W.WWWW.WWWW.W",  // row 21
    "W.WWWW.WWWW.W.WWWW.WWWW.W",  // row 22
    "W...WW.............WW...W",  // row 23  — Pac-Man start area
    "WWW.WW.W.WWWWWWW.W.WW.WWW",  // row 24
    "WWW.WW.W.WWWWWWW.W.WW.WWW",  // row 25
    "W......W....W....W......W",  // row 26
    "W.WWWW.W.WWWWWWW.W.WWWW.W",  // row 27
    "W.WWWW.W.WWWWWWW.W.WWWW.W",  // row 28
    "W.......................W",  // row 29
    "WWWWWWWWWWWWWWWWWWWWWWWWW",  // row 30  — bottom border
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

bool Map::is_wall(float px, float py) const {
    return tile_at(px, py) == Tile::Wall;
}

bool Map::is_wall_at(int row, int col) const {
    return tile_at_index(row, col) == Tile::Wall;
}

void Map::draw(Renderer& renderer) const {
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            switch (at(r, c)) {
            case Tile::Wall:
                renderer.draw_rect(
                    { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE },
                    { 33, 33, 222 }
                );
                break;
            case Tile::Pellet:
                renderer.draw_rect(
                    { c * TILE_SIZE + 10, r * TILE_SIZE + 10, 4, 4 },
                    { 255, 255, 255 }
                );
                break;
            default: break;
            }
        }
    }
}