module;
#include <string_view>

export module game.console;

import game.types;

export class ConsoleView {
public:
    void toggle() { visible_ = !visible_; }
    [[nodiscard]] bool is_visible() const { return visible_; }

    void draw(const GameState& game_state);

private:
    void dispatch_command(std::string_view cmd, const GameState& game_state);

    bool visible_ = true;
    bool show_level_[4] { true, true, true, true };   // Trace, Info, Warn, Error
    char filter_[128] {};
    char input_[256] {};
    bool auto_scroll_ = true;
    bool scroll_to_bottom_ = false;
};
