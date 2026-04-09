export module game.pacman;

import engine.input;
import engine.renderer;
import engine.types;
import game.concepts;
import game.map;

export class Pacman {
public:
	Pacman(const Map& map);

	void reset();

	void draw(Renderer& renderer);

	void handleInput(const InputState& input);

	void update(float dt);

	AABB getBounds() const;

private:
	const Map& map_;

	float x_, y_; // world position in pixels

	float dx_, dy_; // current velocity in pixels per second
	float speed_ = 4.f;
};

static_assert(GameEntity<Pacman>, "Pacman must satisfy GameEntity");