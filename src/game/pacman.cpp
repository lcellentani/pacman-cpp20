module;
#include <cassert>
#include <cmath>

module game.pacman;

void Pacman::reset(const Map* map) {
	map_ = map;

	col_ = 13;
	row_ = 23;
	offset_ = 0;

	accumulator_ = 0.0f;
	speed_ = 40.0f;

	current_dir_ = { 0, 0 };
	queued_dir_ = { 0, 0 };
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
		queued_dir_ = { 0, -1 };
	}
	if (input.down) {
		queued_dir_ = { 0, 1 };
	}
	if (input.left) {
		queued_dir_ = { -1, 0 };
	}
	if (input.right) {
		queued_dir_ = { 1, 0 };
	}
}

void Pacman::update(float dt) {
	accumulator_ += speed_ * dt;

	// If player wants to go exactly opposite, don't wait for the tile center.
	if (is_opposite(queued_dir_, current_dir_)) {
		col_ += current_dir_.x;   // step into the tile we were heading toward
		row_ += current_dir_.y;
		current_dir_ = queued_dir_;
		offset_ = TILE_SIZE - offset_;
	}

	while (accumulator_ >= 1.0f) {
		accumulator_ -= 1.0f;

		if (offset_ == 0) {
			// At a tile center — this is the only moment decisions are made
			if (can_move(col_, row_, queued_dir_)) {
				current_dir_ = queued_dir_;
			}
			if (!can_move(col_, row_, current_dir_)) {
				current_dir_ = { 0, 0 }; // Hit a wall, stop movement
				break;
			}
		}

		if (current_dir_.x == 0 && current_dir_.y == 0)
			break;  // no direction — don't advance offset

		// Advance one pixel in the current direction
		offset_ += 1;

		if (offset_ == TILE_SIZE) {
			// Completed crossing into the next tile
			col_ += current_dir_.x;
			row_ += current_dir_.y;
			offset_ = 0;
		}
	}
}

AABB Pacman::getBounds() const {
	constexpr float margin = 2.f;
	return { pixel_x() + margin, pixel_y() + margin, TILE_SIZE - margin * 2, TILE_SIZE - margin * 2};
}

bool Pacman::is_at_tile_center() const {
	return offset_ == 0;
}

PacmanDebugState Pacman::debug_state() const {
	return { col_, row_, current_dir_.x, current_dir_.y, speed_, getBounds()};
}

int Pacman::pixel_x() const {
	return col_ * TILE_SIZE + offset_ * current_dir_.x;
}

int Pacman::pixel_y() const {
	return row_ * TILE_SIZE + offset_ * current_dir_.y;
}

bool Pacman::is_opposite(const Dir& a, const Dir& b) const {
	if (a.x == 0 && a.y == 0) return false; // no direction is not opposite to anything
	return (a.x == -b.x && a.y == 0) || (a.y == -b.y && a.x == 0);
}

bool Pacman::can_move(int col, int row, Dir dir) const {
	assert(map_ && "can_move called before reset()");
	return !map_->is_wall_at(col + dir.x, row + dir.y);
}
