module engine.game;

// W = wall, P = pellet, . = empty
// Phase 1: a simple but recognisable Pac-Man layout
static constexpr TileMap build_map() {
    using T = Tile;
    TileMap m{};
    // outer walls
    for (int c = 0; c < MAP_COLS; ++c) {
        m[0][c] = m[MAP_ROWS - 1][c] = T::Wall;
    }
    for (int r = 0; r < MAP_ROWS; ++r) {
        m[r][0] = m[r][MAP_COLS - 1] = T::Wall;
    }
    // fill interior with pellets
    for (int r = 1; r < MAP_ROWS - 1; ++r)
        for (int c = 1; c < MAP_COLS - 1; ++c)
            m[r][c] = T::Pellet;
    // a few interior walls for structure
    for (int c = 3; c < 8; ++c)  m[3][c] = T::Wall;
    for (int c = 13; c < 18; ++c) m[3][c] = T::Wall;
    for (int r = 3; r < 8; ++r)  m[r][10] = T::Wall;
    return m;
}

Game::Game()
    : map_(build_map())
    , player_{ TILE_SIZE * 1.5f, TILE_SIZE * 1.5f }
{
}

void Game::update(const InputState& input) {
    if (input.quit) { running_ = false; return; }

    float dx = 0.f, dy = 0.f;
    if (input.up) dy -= player_.speed;
    if (input.down) dy += player_.speed;
    if (input.left) dx -= player_.speed;
    if (input.right) dx += player_.speed;

    // Naive collision: only move if destination tile is not a wall
    auto tile_at = [&](float px, float py) -> Tile {
        int col = static_cast<int>(px) / TILE_SIZE;
        int row = static_cast<int>(py) / TILE_SIZE;
        if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
            return Tile::Wall;
        return map_[row][col];
        };

    if (tile_at(player_.x + dx, player_.y) != Tile::Wall) player_.x += dx;
    if (tile_at(player_.x, player_.y + dy) != Tile::Wall) player_.y += dy;
}

void Game::render(Renderer& renderer) {
    renderer.clear({ 0, 0, 0 });
    render_map(renderer);
    render_player(renderer);
    renderer.present();
}

void Game::render_map(Renderer& renderer) {
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

void Game::render_player(Renderer& renderer) {
    renderer.draw_rect(
        { static_cast<int>(player_.x), static_cast<int>(player_.y),
          TILE_SIZE - 4, TILE_SIZE - 4 },
        { 255, 255, 0 }   // Pac-Man yellow
    );
}