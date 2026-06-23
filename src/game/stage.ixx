module;

export module game.stage;

import engine.renderer;
import engine.input;
import game.concepts;
import game.config;
import game.console;
import game.debug;
import game.ghost;
import game.map;
import game.pacman;
import game.scheduler;
import game.types;

export class Stage {
public:
    Stage(GameConfig config = {});

    void reset();

    void update(const InputState& input, float dt);
    void render(Renderer& renderer);
    [[nodiscard]] bool is_running() const { return running_; }
    [[nodiscard]] int score() const { return score_; }
    void increment_score(int delta);

private:
    Map map_;
	Pacman pacman_;
    Ghost blinky_;
    Ghost pinky_;
    Ghost inky_;
    Ghost clyde_;

    GameState game_state_ {};
    GameConfig config_;

    Scheduler scheduler_;

    bool running_ = false;
    int score_ = 0;

    DebugView debug_;
    ConsoleView console_;
    bool prev_debug_key_ = false;
    bool prev_console_key_ = false;

    void eat_dot(int col, int row);
};