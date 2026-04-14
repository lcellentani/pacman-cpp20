
import engine.renderer;
import engine.input;
import game.stage;
import game.types;

int main() {
    Renderer renderer{ "Pac-Man", WINDOW_W + DEBUG_PANEL_W, WINDOW_H };
    Stage stage{};

    stage.reset();

    while (stage.is_running()) {
        const auto input = poll_input();

        stage.update(input);
        stage.render(renderer);
    }

    return 0;
}