module game.map;

Map::Map() : map_{} {
	// Initialize the map with empty tiles
	for (auto& row : map_)
		for (auto& tile : row)
			tile = Tile::Empty;
}

// W = wall, P = pellet, . = empty
void Map::reset() {
    using T = Tile;
    
    // outer walls
    for (int c = 0; c < MAP_COLS; ++c) {
        map_[0][c] = map_[MAP_ROWS - 1][c] = T::Wall;
    }
    for (int r = 0; r < MAP_ROWS; ++r) {
        map_[r][0] = map_[r][MAP_COLS - 1] = T::Wall;
    }
    // fill interior with pellets
    for (int r = 1; r < MAP_ROWS - 1; ++r)
        for (int c = 1; c < MAP_COLS - 1; ++c)
            map_[r][c] = T::Pellet;
    // a few interior walls for structure
    for (int c = 3; c < 8; ++c)  map_[3][c] = T::Wall;
    for (int c = 13; c < 18; ++c) map_[3][c] = T::Wall;
    for (int r = 3; r < 8; ++r)  map_[r][10] = T::Wall;
}

Tile Map::tile_at(float px, float py) const {
    int col = static_cast<int>(px) / TILE_SIZE;
    int row = static_cast<int>(py) / TILE_SIZE;
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
        return Tile::Wall; // treat out-of-bounds as walls
    return map_[row][col];
}

void Map::draw(Renderer& renderer) const {
    for (int r = 0; r < MAP_ROWS; ++r) {
        for (int c = 0; c < MAP_COLS; ++c) {
            switch (map_[r][c]) {
            case Tile::Wall:
                renderer.draw_rect(
                    { c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE },
                    { 33, 33, 222 }   // classic Pac-Man blue
                );
                break;
            case Tile::Pellet:
                renderer.draw_rect(
                    { c * TILE_SIZE + 14, r * TILE_SIZE + 14, 4, 4 },
                    { 255, 255, 255 }
                );
                break;
            default: break;
            }
        }
    }
}