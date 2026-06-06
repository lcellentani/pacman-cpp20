module;
#include <coroutine>
#include <utility>

module game.ghost;

import engine.log;
import game.scheduler;

struct move_to {
    Ghost& ghost_;
    Scheduler& scheduler_;
    std::pair<int, int> target_;
    std::coroutine_handle<> handle_;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        log_trace("about to register updatable");
        scheduler_.register_updatable(*this);
    }

    void await_resume() {}

    move_to(Ghost& ghost, Scheduler& scheduler, std::pair<int, int> target)
        : ghost_(ghost), scheduler_(scheduler), target_(std::move(target)) {}

    void update(float dt) {
        ghost_.move_toward(target_.first, target_.second, dt);
        if (ghost_.ghost_reached(target_.first, target_.second)) {
            handle_.resume();
        }
    }
};

void Ghost::reset(Scheduler& scheduler) {
    behavior_ = wander(scheduler);
    behavior_.handle_.resume();
}

Task Ghost::wander(Scheduler& scheduler) {
    while (true) {
        std::pair<int, int> target = pick_random_target();
        co_await move_to(*this, scheduler, target);
    }
    co_return;
}

std::pair<int, int> Ghost::pick_random_target() {
    return { 0, 0 };
}

void Ghost::move_toward([[maybe_unused]] int col, [[maybe_unused]] int row,
                        [[maybe_unused]] float dt) {}

bool Ghost::ghost_reached([[maybe_unused]] int col, [[maybe_unused]] int row) {
    return false;
}
