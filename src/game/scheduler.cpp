module;
#include <unordered_set>
#include <vector>

module game.scheduler;

void Scheduler::unregister_updatable(Handle handle) {
    removals_.insert(handle);
}

void Scheduler::update(float dt) {
    // Clean up removals from previous frame or current requests
    if (!removals_.empty()) {
        std::erase_if(updatables_, [this](const Entry& e) {
            return removals_.contains(e.h);
        });
        removals_.clear();
    }

    // Use index-based loop in case register_updatable is called during update
    size_t count = updatables_.size();
    for (size_t i = 0; i < count; ++i) {
        auto& entry = updatables_[i];
        if (!removals_.contains(entry.h)) {
            entry.fn(dt);
        }
    }
}
