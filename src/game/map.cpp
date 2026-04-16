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
    "WWWWWW.WW.WWWWWWWW.WW.WWWWWW",  // row 12
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

bool Map::is_wall_at(int col, int row) const {
    return tile_at_index(row, col) == Tile::Wall;
}

bool Map::collect_pellet_at(int col, int row) {
    if (tile_at_index(row, col) == Tile::Pellet) {
        at(row, col) = Tile::Empty;
        return true;
    }
    if (tile_at_index(row, col) == Tile::SuperPellet) {
        at(row, col) = Tile::Empty;
        return true;
    }
	return false;
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
            case Tile::SuperPellet:
                renderer.draw_circle(
                    c * TILE_SIZE + HALF_TILE_SIZE,
                    r * TILE_SIZE + HALF_TILE_SIZE,
                    7, 
                    { 255, 255, 255 });
                break;
            default: break;
            }
        }
    }
}