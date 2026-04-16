export module game.pacman;

import engine.input;
import engine.renderer;
import engine.types;
import game.concepts;
import game.map;
import game.types;

export class Pacman {
public:
	Pacman() = default;

	void reset(Map* map);

	void draw(Renderer& renderer);

	void handleInput(const InputState& input);

	void update(float dt);

	AABB getBounds() const;

	bool is_at_tile_center() const;

	int current_col() const { return col_; }
	int current_row() const { return row_; }

	[[nodiscard]] PacmanDebugState debug_state() const;

private:
	int col_ = 0; // current tile column
	int row_ = 0; // current tile row
	int offset_ = 0; // pixel offset from tile center (0 to TILE_SIZE/2)

	Dir current_dir_{ 0, 0 }; // current movement direction (normalized)
	Dir queued_dir_{ 0, 0 }; // queued movement direction (normalized)

	float accumulator_ = 0.f; // accumulator for movement timing
	float speed_ = 0.0f;

	Map* map_ = nullptr; // pointer, rebindable, nullable

	int pixel_x() const;
	int pixel_y() const;

	bool is_opposite(const Dir& a, const Dir& b) const;
	bool can_move(int col, int row, Dir dir) const;
};

static_assert(GameEntity<Pacman>, "Pacman must satisfy GameEntity");