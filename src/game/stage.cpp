module;
#include <array>
#include "imgui.h"

module game.stage;

import engine.log;
import game.types;

Stage::Stage(GameConfig config)
	: blinky_(GhostId::Blinky), pinky_(GhostId::Pinky), inky_(GhostId::Inky), clyde_(GhostId::Clyde), config_(config) {
}

void Stage::reset() {
	map_.reset();
	pacman_.reset(&map_);
	blinky_.reset(scheduler_, &map_, &game_state_);
	pinky_.reset(scheduler_, &map_, &game_state_);
	inky_.reset(scheduler_, &map_, &game_state_);
	clyde_.reset(scheduler_, &map_, &game_state_);

    running_ = true;
}

void Stage::increment_score(int delta) {
    score_ += delta;
}

void Stage::update(const InputState& input, float dt) {
	if (!running_) return;

    if (input.quit) { running_ = false; return; }

	// Edge detection — toggle only on keydown, not keyhold
	if (input.debug_toggle && !prev_debug_key_)
		debug_.toggle();
	prev_debug_key_ = input.debug_toggle;

	if (input.console_toggle && !prev_console_key_)
		console_.toggle();
	prev_console_key_ = input.console_toggle;

	pacman_.handle_input(input);

	pacman_.set_speed(config_.pacman_speed);
	pacman_.update(dt);

	scheduler_.update(dt);

	if (pacman_.is_at_tile_center()) {
		int pac_col = pacman_.current_col();
		int pac_row = pacman_.current_row();
		if (map_.tile_at_index(pac_row, pac_col) == Tile::Pellet) {
			map_.clear_tile(pac_col, pac_row);
		}
		else if (map_.tile_at_index(pac_row, pac_col) == Tile::SuperPellet) {
			map_.clear_tile(pac_col, pac_row);
		}
	}

	game_state_.pacman_tile = pacman_.current_coord();
	game_state_.pacman_dir = pacman_.current_dir();
	game_state_.blinky_tile = blinky_.current_coord();
}

void Stage::render(Renderer& renderer) {
	// Game world is drawn into an off-screen SDL_Texture, then shown
	// inside the ImGui "Game" panel.
	renderer.begin_game_target();
	renderer.clear({ 0, 0, 0, 255 });
	map_.draw(renderer);
	pacman_.draw(renderer);
	blinky_.draw(renderer);
	pinky_.draw(renderer);
	inky_.draw(renderer);
	clyde_.draw(renderer);
	renderer.end_game_target();

	renderer.imgui_new_frame();
	renderer.clear({ 20, 20, 20, 255 });   // window background behind panels

	ImGui::SetNextWindowPos({ (float)LAYOUT_GAME_X, (float)LAYOUT_GAME_Y }, ImGuiCond_Once);
	ImGui::Begin("Game", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Image(reinterpret_cast<ImTextureID>(renderer.game_texture_id()),
		ImVec2{ (float)renderer.game_target_width(), (float)renderer.game_target_height() });
	ImGui::End();

	std::array<GhostDebugState, 4> ghosts{ blinky_.debug_state(), pinky_.debug_state(), inky_.debug_state(), clyde_.debug_state() };
	debug_.draw(map_, pacman_.debug_state(), ghosts, config_);
	console_.draw();

	renderer.imgui_render();
	renderer.present();
}
