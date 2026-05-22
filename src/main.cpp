
#include <chrono>

import engine.renderer;
import engine.input;
import game.config;
import game.stage;
import game.types;

int main() {
    Renderer renderer{ "Pac-Man", WINDOW_W + DEBUG_PANEL_W, WINDOW_H };
    Stage stage{ load_config("config.json") };

    stage.reset();

    auto last_time = std::chrono::steady_clock::now();

    while (stage.is_running()) {
        const auto input = poll_input();

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        dt = std::min(dt, 0.05f);

        stage.update(input, dt);
        stage.render(renderer);
    }

    return 0;
}