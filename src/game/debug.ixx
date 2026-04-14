module;
#include "imgui.h"

export module game.debug;

import engine.renderer;
import game.map;
import game.types;

export class DebugView {
public:
    void toggle() { visible_ = !visible_; }
    [[nodiscard]] bool is_visible() const { return visible_; }

    void draw(const Map& map, const PacmanDebugState& pacman);

private:
    void draw_pacman_section(const PacmanDebugState& pacman);
    void draw_map_section(const Map& map);

    bool visible_ = false;
};