module;
#include <variant>
#include <vector>

export module game.stage;

import engine.renderer;
import engine.input;
import game.concepts;
import game.map;
import game.pacman;

using DrawableObject = std::variant<Pacman, Map>;

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

    std::vector<DrawableObject> drawableObjects_;

    bool running_ = false;
};