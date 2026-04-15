module game.pacman;

Pacman::Pacman(const Map& map) :
	map_(map),
	dx_(0.f), dy_(0.f) {
	// Initial position will be set in reset().
}

void Pacman::reset() {
	// Placeholder for reset logic (e.g., position, state).
	pos_.x = 12 * TILE_SIZE;
	pos_.y = 23 * TILE_SIZE;

	accumulator_ = 0.0f;
	speed_ = 40.0f;
	current_dir_ = { 0.f, 0.f };
	queued_dir_ = { 0.f, 0.f };
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
	
	if (input.up) {
		dy_ -= speed_;
		current_dir_ = { 0.f, -1.f };
	}
	if (input.down) {
		dy_ += speed_;
		current_dir_ = { 0.f, 1.f };
	}
	if (input.left) {
		dx_ -= speed_;
		current_dir_ = { -1.f, 0.f };
	}
	if (input.right) {
		dx_ += speed_;
		current_dir_ = { 1.f, 0.f };
	}
}

void Pacman::resolveWorldCollisions(const Map& map) {
	if (map.is_wall(pos_.x + dx_, pos_.y)) dx_ = 0.f;
	if (map.is_wall(pos_.x, pos_.y + dy_)) dy_ = 0.f;
}

void Pacman::update(float dt) {
	//pos_.x += dx_ * dt;
	//pos_.y += dy_ * dt;

	accumulator_ += speed_ * dt;
	while (accumulator_ >= 1.0f) {
		if (is_wall(pos_, current_dir_)) {
			// Hit a wall, stop movement
			current_dir_ = { 0.f, 0.f };
			accumulator_ = 0.0f; // reset accumulator to prevent further movement until next input
			break;
		}
		pos_.x += current_dir_.x;
		pos_.y += current_dir_.y;
		accumulator_ -= 1.0f;
	}
}

AABB Pacman::getBounds() const {
	constexpr float margin = 2.f;
	return { pos_.x + margin, pos_.y + margin,
			 TILE_SIZE - margin * 2, TILE_SIZE - margin * 2 };
}

PacmanDebugState Pacman::debug_state() const {
	return { pos_.x, pos_.y, dx_, dy_, speed_, getBounds() };
}

bool Pacman::is_wall(Vec2 pos, Vec2 dir) const {
	const float next_x = pos.x + dir.x;
	const float next_y = pos.y + dir.y;
	return map_.is_wall(next_x, next_y);
}