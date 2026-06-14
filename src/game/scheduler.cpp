module;
#include <algorithm>
#include <vector>

module game.scheduler;

void Scheduler::unregister_updatable(Handle handle) {
    removals_.push_back(handle);
}

void Scheduler::update(float dt) {
    // Clean up removals from previous frame or current requests
    if (!removals_.empty()) {
        std::erase_if(updatables_, [this](const Entry& e) {
            return std::any_of(removals_.begin(), removals_.end(), 
                               [&](Handle h) { return h == e.h; });
        });
        removals_.clear();
    }

    // Use index-based loop in case register_updatable is called during update
    size_t count = updatables_.size();
    for (size_t i = 0; i < count; ++i) {
        auto& entry = updatables_[i];
        
        // Skip if this handle was just unregistered in this frame
        bool is_removed = std::any_of(removals_.begin(), removals_.end(), 
                                      [&](Handle h) { return h == entry.h; });
        
        if (!is_removed) {
            entry.fn(dt);
        }
    }
}
