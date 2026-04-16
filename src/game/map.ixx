module;
#include <array>
#include <cstdint>

export module game.map;

import engine.renderer;
import game.types;

// Tile types
export enum class Tile : uint8_t {
    Empty = 0,
    Wall = 1,
    Pellet = 2,
	SuperPellet = 3,
};

export class Map {
public:
    Map();

    void reset();

    Tile tile_at(float px, float py) const;
    [[nodiscard]] Tile tile_at_index(int row, int col) const;

    bool is_wall(float px, float py) const;
	bool is_wall_at(int col, int row) const;

	bool collect_pellet_at(int col, int row);

	void draw(Renderer& renderer) const;

private:
    // Flat storage — index with at(row, col)
    std::array<Tile, MAP_ROWS* MAP_COLS> map_;

    [[nodiscard]] Tile& at(int row, int col) {
        return map_[row * MAP_COLS + col];
    }
    [[nodiscard]] Tile at(int row, int col) const {
        return map_[row * MAP_COLS + col];
    }
};