module;
#include <cmath>

module game.pacman;

Pacman::Pacman(const Map& map) : map_(map) {
	// Initial position will be set in reset().
}

void Pacman::reset() {
	// Placeholder for reset logic (e.g., position, state).
	pos_.x = 12 * TILE_SIZE + HALF_TILE_SIZE;
	pos_.y = 23 * TILE_SIZE + HALF_TILE_SIZE;

	accumulator_ = 0.0f;
	speed_ = 40.0f;
	current_dir_ = { 1.f, 0.f };
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
	if (input.up) {
		queued_dir_ = { 0.f, -1.f };
	}
	if (input.down) {
		queued_dir_ = { 0.f, 1.f };
	}
	if (input.left) {
		queued_dir_ = { -1.f, 0.f };
	}
	if (input.right) {
		queued_dir_ = { 1.f, 0.f };
	}
}

void Pacman::resolveWorldCollisions(const Map& map) {
	//if (map.is_wall(pos_.x + dx_, pos_.y)) dx_ = 0.f;
	//if (map.is_wall(pos_.x, pos_.y + dy_)) dy_ = 0.f;
}

void Pacman::update(float dt) {
	accumulator_ += speed_ * dt;
	while (accumulator_ >= 1.0f) {
		if (is_at_tile_center(pos_)) {
			if (can_move(pos_, queued_dir_)) {
				current_dir_ = queued_dir_;  // commit to the queued direction at tile centers
				pos_.x = floor(pos_.x / TILE_SIZE) * TILE_SIZE + HALF_TILE_SIZE;
				pos_.y = floor(pos_.y / TILE_SIZE) * TILE_SIZE + HALF_TILE_SIZE;
			}
			else if (is_wall(pos_, current_dir_)) {
				// Hit a wall, stop movement
				current_dir_ = { 0.f, 0.f };
				queued_dir_ = { 0.f, 0.f };
				accumulator_ = 0.0f; // reset accumulator to prevent further movement until next input
				break;
			}
		}
		pos_.x += current_dir_.x;
		pos_.y += current_dir_.y;
		accumulator_ -= 1.0f;
	}
}

AABB Pacman::getBounds() const {
	constexpr float margin = 2.f;
	float x = pos_.x - HALF_TILE_SIZE + margin;
	float y = pos_.y - HALF_TILE_SIZE + margin;
	return { x, y, TILE_SIZE - margin * 2, TILE_SIZE - margin * 2 };
}

PacmanDebugState Pacman::debug_state() const {
	return { pos_.x, pos_.y, current_dir_.x, current_dir_.y, speed_, getBounds() };
}

bool Pacman::is_wall(Vec2 pos, Vec2 dir) const {
	// Look ahead to the next tile
	float nextX = pos.x + (dir.x * TILE_SIZE);
	float nextY = pos.y + (dir.y * TILE_SIZE);
	return map_.is_wall(nextX, nextY);
}

bool Pacman::is_at_tile_center(Vec2 pos) const {
	int ix = static_cast<int>(pos.x);
	int iy = static_cast<int>(pos.y);
	return ((ix % TILE_SIZE) == HALF_TILE_SIZE) && ((iy % TILE_SIZE) == HALF_TILE_SIZE);
}

bool Pacman::can_move(Vec2 pos, Vec2 dir) const {
	float targetX = pos.x + (dir.x * TILE_SIZE);
	float targetY = pos.y + (dir.y * TILE_SIZE);
	return !map_.is_wall(targetX, targetY);
}