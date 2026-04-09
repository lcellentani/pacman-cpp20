export module game.pacman;

import engine.input;
import engine.renderer;
import engine.types;
import game.concepts;

export class Pacman {
public:
	void draw(Renderer& r);

	void handleInput(const InputState& input);

	void update(float dt, const InputState& input);

	AABB getBounds() const;
};

static_assert(GameEntity<Pacman>, "Pacman must satisfy GameEntity");