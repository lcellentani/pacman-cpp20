
import engine.renderer;
import engine.input;
import game.game_manager;

int main() {
    Renderer renderer{ "Pac-Man", 672, 672 };  // 21 * 32
    GameManager game{};

    while (game.is_running()) {
        const auto input = poll_input();
        game.update(input);
        game.render(renderer);
    }

    return 0;
}