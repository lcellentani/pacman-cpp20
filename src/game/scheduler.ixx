module;
#include <functional>
#include <vector>

export module game.scheduler;

import game.concepts;

export class Scheduler {
public:
    template<Updatable T>
    void register_updatable(T& entity) {
        updatables_.push_back([&entity](float dt) { entity.update(dt); });
    }

    void update(float dt);

private:
    std::vector<std::function<void(float)>> updatables_;
};
