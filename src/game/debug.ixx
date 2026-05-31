module;
#include "imgui.h"
#include <span>

export module game.debug;

import engine.renderer;
import game.config;
import game.map;
import game.types;

export class DebugView {
public:
    void toggle() { visible_ = !visible_; }
    [[nodiscard]] bool is_visible() const { return visible_; }

    void draw(const Map& map, const PacmanDebugState& pacman,
              std::span<const GhostDebugState> ghosts, GameConfig& config);

private:
    void draw_pacman_section(const PacmanDebugState& pacman);
    void draw_ghost_section(const GhostDebugState& ghost);
    void draw_map_section(const Map& map, const PacmanDebugState& pacman);
    void draw_tweaks_section(GameConfig& config);

    bool visible_ = false;
};