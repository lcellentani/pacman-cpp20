module game.pacman;

Pacman::Pacman() :
	x_(0.f), y_(0.f),
	dx_(0.f), dy_(0.f) {
	// Initial position will be set in reset().
}

void Pacman::reset() {
	// Placeholder for reset logic (e.g., position, state).
	x_ = 12 * TILE_SIZE;
	y_ = 23 * TILE_SIZE;
}

void Pacman::draw(Renderer& renderer) {
	renderer.draw_rect(
		{ static_cast<int>(x_) + 2, static_cast<int>(y_) + 2,
		  TILE_SIZE - 4, TILE_SIZE - 4 },
		{ 255, 255, 0 }   // Pac-Man yellow
	);
}

void Pacman::handleInput(const InputState& input) {
	dx_ = 0.f;
	dy_ = 0.f;
	
	if (input.up) dy_ -= speed_;
	if (input.down) dy_ += speed_;
	if (input.left) dx_ -= speed_;
	if (input.right) dx_ += speed_;
}

void Pacman::resolveWorldCollisions(const Map& map) {
	if (map.is_wall(x_ + dx_, y_)) dx_ = 0.f;
	if (map.is_wall(x_, y_ + dy_)) dy_ = 0.f;
}

void Pacman::update(float dt) {
	x_ += dx_ * dt;
	y_ += dy_ * dt;
}

AABB Pacman::getBounds() const {
	// TODO: fix bounds calculation in phase 3
	return { x_, y_, static_cast<float>(TILE_SIZE - 4),
					 static_cast<float>(TILE_SIZE - 4) };
}

PacmanDebugState Pacman::debug_state() const {
	return { x_, y_, dx_, dy_, speed_, getBounds() };
}