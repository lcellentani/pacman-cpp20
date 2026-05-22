module;

export module game.stage;

import engine.renderer;
import engine.input;
import game.concepts;
import game.config;
import game.debug;
import game.map;
import game.pacman;

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
	Pacman pacman_entity_;

    bool running_ = false;
    int score_ = 0;

    GameConfig config_;
    DebugView debug_;
    bool prev_debug_key_ = false;
};