export module game.pacman;

import engine.input;
import engine.renderer;
import engine.types;
import game.concepts;
import game.map;
import game.types;

export class Pacman {
public:
	Pacman(const Map& map);

	void reset();

	void draw(Renderer& renderer);

	void handleInput(const InputState& input);

	void resolveWorldCollisions(const Map& map);

	void update(float dt);

	AABB getBounds() const;

	[[nodiscard]] PacmanDebugState debug_state() const;

private:
	Vec2 pos_{ 0.0f, 0.0f }; // current position in pixels

	Vec2 current_dir_{ 0.0f, 0.0f }; // current movement direction (normalized)
	Vec2 queued_dir_{ 0.0f, 0.0f }; // queued movement direction (normalized)

	float dx_, dy_; // current velocity in pixels per second

	float accumulator_ = 0.f; // accumulator for movement timing
	float speed_ = 0.0f;

	const Map& map_;

	bool is_wall(Vec2 pos, Vec2 dir) const;
};

static_assert(GameEntity<Pacman>, "Pacman must satisfy GameEntity");