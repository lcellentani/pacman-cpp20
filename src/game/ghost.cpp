module;
#include <coroutine>
#include <string>
#include <cassert>

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

    switch (id_) {
    case GhostId::Blinky: col_ = 13; row_ = 11; break;
    case GhostId::Pinky:  col_ = 13; row_ = 14; break;
    case GhostId::Inky:   col_ = 11; row_ = 14; break;
    case GhostId::Clyde:  col_ = 15; row_ = 14; break;
    }
	offset_ = 0;

	accumulator_ = 0.0f;
	speed_ = 150.0f;

	current_dir_ = { 0, 0 };

    behavior_ = wander(scheduler);
    behavior_.handle_.resume();
}

void Ghost::draw(Renderer& renderer) {
    const auto b = get_bounds();
    renderer.draw_rect(
        { static_cast<int>(b.x), static_cast<int>(b.y),
          static_cast<int>(b.width), static_cast<int>(b.height) },
        get_color()
    );
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

int Ghost::pixel_x() const {
    return col_ * TILE_SIZE + offset_ * current_dir_.x;
}

int Ghost::pixel_y() const {
    return row_ * TILE_SIZE + offset_ * current_dir_.y;
}

bool Ghost::can_move(int col, int row, Dir dir) const {
    assert(map_ && "can_move called before reset()");
	return !map_->is_wall_at(col + dir.x, row + dir.y);
}

MapCoord Ghost::pick_random_target() {
    MapCoord t = map_->pick_random_walkable();
    log_trace("pick_random_target -> " + std::to_string(t.col) + "," + std::to_string(t.row));
    return t;
}

void Ghost::move_toward([[maybe_unused]] MapCoord target, [[maybe_unused]] float dt) {
    // To be implemented in Phase 3
}

bool Ghost::ghost_reached([[maybe_unused]] MapCoord target) {
    return false;
}

AABB Ghost::get_bounds() const {
    constexpr float margin = 2.f;
    return { pixel_x() + margin, pixel_y() + margin, TILE_SIZE - margin * 2, TILE_SIZE - margin * 2};
}
