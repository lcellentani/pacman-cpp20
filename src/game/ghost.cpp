module;
#include <coroutine>
#include <string>

module game.ghost;

import engine.log;
import game.map;
import game.scheduler;
import game.types;

struct move_to {
    Ghost& ghost_;
    Scheduler& scheduler_;
    MapCoord target_;
    std::coroutine_handle<> handle_;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        log_trace("about to register updatable");
        scheduler_.register_updatable(*this);
    }

    void await_resume() {}

    move_to(Ghost& ghost, Scheduler& scheduler, MapCoord target)
        : ghost_(ghost), scheduler_(scheduler), target_(target) {}

    void update(float dt) {
        ghost_.move_toward(target_, dt);
        if (ghost_.ghost_reached(target_)) {
            handle_.resume();
        }
    }
};

Ghost::Ghost(GhostId id) : id_(id) {

}

void Ghost::reset(Scheduler& scheduler, const Map* map) {
    map_ = map;

    behavior_ = wander(scheduler);
    behavior_.handle_.resume();
}

Color Ghost::get_color() const {
    switch (id_) {
    case GhostId::Blinky: return { 255, 0, 0, 255 };
    case GhostId::Pinky:  return { 255, 184, 255, 255 };
    case GhostId::Inky:   return { 0, 255, 255, 255 };
    case GhostId::Clyde:  return { 255, 184, 82, 255 };
    default: return { 0, 0, 0, 255 };
    }
}

GhostDebugState Ghost::debug_state() const {
    return { id_, { 0, 0 }, 0, 0, 0.0f,{ 0.0f, 0.0f, 0.0f, 0.0f }, get_color(), GhostState::Chase, target_ };
}

Task Ghost::wander(Scheduler& scheduler) {
    while (true) {
        target_ = pick_random_target();
        co_await move_to(*this, scheduler, target_);
    }
    co_return;
}

MapCoord Ghost::pick_random_target() {
    MapCoord t = map_->pick_random_walkable();
    log_trace("pick_random_target -> " + std::to_string(t.col) + "," + std::to_string(t.row));
    return t;
}

void Ghost::move_toward([[maybe_unused]] MapCoord target, [[maybe_unused]] float dt) {
}

bool Ghost::ghost_reached([[maybe_unused]] MapCoord target) {
    return false;
}
