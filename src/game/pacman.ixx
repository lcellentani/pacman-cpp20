export module game.pacman;

import engine.input;
import engine.renderer;
import engine.types;
import game.concepts;
import game.map;

export class Pacman {
public:
	Pacman();

	void reset();

	void draw(Renderer& renderer);

	void handleInput(const InputState& input);

	void resolveWorldCollisions(const Map& map);

	void update(float dt);

	AABB getBounds() const;

private:
	float x_, y_; // world position in pixels

	float dx_, dy_; // current velocity in pixels per second
	float speed_ = 4.f;
};

static_assert(GameEntity<Pacman>, "Pacman must satisfy GameEntity");