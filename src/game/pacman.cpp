module game.pacman;

Pacman::Pacman(const Map& map)
	: map_(map),
	x_(0.f), y_(0.f),
	dx_(0.f), dy_(0.f) {
	// Initial position will be set in reset().
}

void Pacman::reset() {
	// Placeholder for reset logic (e.g., position, state).
	x_ = TILE_SIZE * 1.5f;
	y_ = TILE_SIZE * 1.5f;
}

void Pacman::draw(Renderer& renderer) {
	renderer.draw_rect(
		{ static_cast<int>(x_), static_cast<int>(y_),
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

void Pacman::update(float dt) {
	if (map_.tile_at(x_ + dx_, y_) != Tile::Wall)
		x_ += dx_;
	if (map_.tile_at(x_, y_ + dy_) != Tile::Wall)
		y_ += dy_;
}

AABB Pacman::getBounds() const {
	return { 100, 100, 32, 32 }; // Placeholder bounds for collision detection.
}