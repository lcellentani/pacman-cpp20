module;
#include <array>
#include <cstdint>

export module game.map;

import engine.renderer;

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

export class Map {
public:
    Map();

    void reset();

    Tile tile_at(float px, float py) const;

	void draw(Renderer& renderer) const;

private:
    using TileMap = std::array<std::array<Tile, MAP_COLS>, MAP_ROWS>;
	TileMap map_;
};