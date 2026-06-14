module;
#include <functional>
#include <unordered_set>
#include <vector>

export module game.scheduler;

import game.concepts;

export class Scheduler {
public:
    using Handle = uint32_t;

    template<Updatable T>
    [[nodiscard]] Handle register_updatable(T& entity) {
        Handle h = next_handle_++;
        updatables_.push_back({ h, [&entity](float dt) { entity.update(dt); } });
        return h;
    }

    void unregister_updatable(Handle handle);
    
    void update(float dt);

private:
    struct Entry { Handle h; std::function<void(float)> fn; };
    std::vector<Entry> updatables_;
    std::unordered_set<Handle> removals_;
    Handle next_handle_ = 1;
};
