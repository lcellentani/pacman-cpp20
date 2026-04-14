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
	const auto b = getBounds();
	renderer.draw_rect(
		{ static_cast<int>(b.x), static_cast<int>(b.y),
		  static_cast<int>(b.width), static_cast<int>(b.height) },
		{ 255, 255, 0 }
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
	constexpr float margin = 2.f;
	return { x_ + margin, y_ + margin,
			 TILE_SIZE - margin * 2, TILE_SIZE - margin * 2 };
}

PacmanDebugState Pacman::debug_state() const {
	return { x_, y_, dx_, dy_, speed_, getBounds() };
}