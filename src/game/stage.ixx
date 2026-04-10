module;

export module game.stage;

import engine.renderer;
import engine.input;
import game.concepts;
import game.map;
import game.pacman;

export class Stage {
public:
    Stage();

    void reset();

    void update(const InputState& input);
    void render(Renderer& renderer);
    [[nodiscard]] bool is_running() const { return running_; }

private:
    Map map_;
	Pacman pacman_entity_;

    bool running_ = false;
};