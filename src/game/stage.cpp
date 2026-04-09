module;
#include <variant>
#include <vector>

module game.stage;

// Overload helper — lets us compose lambdas into a visitor.
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

Stage::Stage()
	: map_(), pacman_entity_(map_) {
}

void Stage::reset() {
	map_.reset();
	pacman_entity_.reset();

	drawableObjects_.clear();
	drawableObjects_.emplace_back(pacman_entity_);
	drawableObjects_.emplace_back(map_);

    running_ = true;
}

void Stage::update(const InputState& input) {
	if (!running_) return;

    if (input.quit) { running_ = false; return; }

	pacman_entity_.handleInput(input);

	pacman_entity_.update(0.f);
}

void Stage::render(Renderer& renderer) {
    renderer.clear({ 0, 0, 0 });

	for (auto& obj : drawableObjects_) {
		std::visit(overloaded{
			[&renderer](Drawable auto& o) { o.draw(renderer); },
			[](auto&) {}
			}, obj);
	}

    renderer.present();
}
