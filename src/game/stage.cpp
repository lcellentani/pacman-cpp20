module;
#include "imgui.h"

module game.stage;

Stage::Stage()
	: map_(), pacman_entity_() {
}

void Stage::reset() {
	map_.reset();
	pacman_entity_.reset();

    running_ = true;
}

void Stage::update(const InputState& input) {
	if (!running_) return;

    if (input.quit) { running_ = false; return; }

	pacman_entity_.handleInput(input);

	pacman_entity_.resolveWorldCollisions(map_);

	pacman_entity_.update(1.f);
}

void Stage::render(Renderer& renderer) {
	renderer.imgui_new_frame(); // ImGui frame starts

	ImGui::ShowDemoWindow();

    renderer.clear({ 0, 0, 0 });

	map_.draw(renderer);
	pacman_entity_.draw(renderer);

	renderer.imgui_render(); // ImGui flushes

    renderer.present();
}
