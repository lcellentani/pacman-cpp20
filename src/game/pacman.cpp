module game.pacman;

void Pacman::draw(Renderer& r) {
	r.draw_rect({ 100, 100, 32, 32 }, { 255, 255, 0 });
}

void Pacman::update(float dt, const InputState& input) {
	// Placeholder for movement logic based on input.
}

AABB Pacman::getBounds() const {
	return { 100, 100, 32, 32 }; // Placeholder bounds for collision detection.
}