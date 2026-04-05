import engine.game;
import engine.renderer;
import engine.input;

int main() {
    Renderer renderer{ "Pac-Man", 672, 672 };  // 21 * 32
    Game game{};

    while (game.is_running()) {
        const auto input = poll_input();
        game.update(input);
        game.render(renderer);
    }

    return 0;
}