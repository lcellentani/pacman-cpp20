module;
//#include "imgui.h"

module game.stage;

Stage::Stage()
	: map_(), pacman_entity_() {
}

void Stage::reset() {
	map_.reset();
	pacman_entity_.reset(&map_);

    running_ = true;
}

void Stage::update(const InputState& input) {
	if (!running_) return;

    if (input.quit) { running_ = false; return; }

	// Edge detection — toggle only on keydown, not keyhold
	if (input.debug_toggle && !prev_debug_key_)
		debug_.toggle();
	prev_debug_key_ = input.debug_toggle;

	pacman_entity_.handleInput(input);

	pacman_entity_.update(0.016f);
}

void Stage::render(Renderer& renderer) {
	renderer.imgui_new_frame(); // ImGui frame starts

	//ImGui::ShowDemoWindow();

    renderer.clear({ 0, 0, 0 });

	map_.draw(renderer);
	pacman_entity_.draw(renderer);

	debug_.draw(map_, pacman_entity_.debug_state());

	renderer.imgui_render(); // ImGui flushes

    renderer.present();
}
