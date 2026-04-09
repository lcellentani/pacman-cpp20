
import engine.renderer;
import engine.input;
import game.stage;

int main() {
    Renderer renderer{ "Pac-Man", 672, 672 };  // 21 * 32
    Stage stage{};

    stage.reset();

    while (stage.is_running()) {
        const auto input = poll_input();

        stage.update(input);
        stage.render(renderer);
    }

    return 0;
}