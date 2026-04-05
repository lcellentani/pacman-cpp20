module;
#include <array>
#include <cstdint>

export module engine.game;

import engine.renderer;
import engine.input;

// Tile types
export enum class Tile : uint8_t {
    Empty = 0,
    Wall = 1,
    Pellet = 2,
};

// Classic Pac-Man map: 21 cols × 21 rows (simplified for Phase 1)
export constexpr int MAP_COLS = 21;
export constexpr int MAP_ROWS = 21;
export constexpr int TILE_SIZE = 32;

using TileMap = std::array<std::array<Tile, MAP_COLS>, MAP_ROWS>;

export struct Player {
    float x, y;       // world position in pixels
    float speed = 4.f;
};

export class Game {
public:
    Game();
    void update(const InputState& input);
    void render(Renderer& renderer);
    [[nodiscard]] bool is_running() const { return running_; }

private:
    void render_map(Renderer& renderer);
    void render_player(Renderer& renderer);

    TileMap map_;
    Player player_;
    bool running_ = true;
};