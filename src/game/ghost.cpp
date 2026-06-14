module;
#include <array>
#include <cassert>
#include <coroutine>
#include <limits>
#include <span>
#include <string>

module game.ghost;

import engine.log;
import engine.random;
import game.map;
import game.scheduler;
import game.types;

struct move_to {
    Ghost& ghost_;
    Scheduler& scheduler_;
    MapCoord target_;
    std::coroutine_handle<> handle_;
    Scheduler::Handle registration_ = 0;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        log_trace("about to register updatable");
        registration_ = scheduler_.register_updatable(*this);
    }

    void await_resume() {
        scheduler_.unregister_updatable(registration_);
    }

    move_to(Ghost& ghost, Scheduler& scheduler, MapCoord target)
        : ghost_(ghost), scheduler_(scheduler), target_(target) {}

    void update(float dt) {
        ghost_.move_toward_greedy(target_, dt);
        if (ghost_.ghost_reached(target_)) {
            handle_.resume();
        }
    }
};

struct walk_path {
    Ghost& ghost_;
    Scheduler& scheduler_;
    std::span<const MapCoord> path_;
    size_t step_ = 0;
    std::coroutine_handle<> handle_;
    Scheduler::Handle registration_ = 0;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        registration_ = scheduler_.register_updatable(*this);
    }

    void await_resume() {
        scheduler_.unregister_updatable(registration_);
    }

    walk_path(Ghost& ghost, Scheduler& scheduler, std::span<const MapCoord> path)
        : ghost_(ghost), scheduler_(scheduler), path_(path) {}

    void update(float dt) {
        ghost_.move_toward(path_[step_], dt);
        if (ghost_.ghost_reached(path_[step_])) {
            step_++;
        }
        if (step_ >= path_.size()) {
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
    speed_ = 50.0f;

    current_dir_ = { 0, 0 };

    behavior_ = behavior(scheduler);
    behavior_.resume();
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
    return { id_, { col_, row_ }, current_dir_.x, current_dir_.y, speed_, get_bounds(), get_color(), GhostState::Chase, target_ };
}

Task Ghost::behavior(Scheduler& scheduler) {
    if (id_ != GhostId::Blinky) {
        co_await walk_path(*this, scheduler, path_for_ghost(id_));
    }
    
    while (true) {
        target_ = pick_random_target();
        co_await move_to(*this, scheduler, target_);
    }
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
    return map_->pick_random_walkable();
}

std::span<const MapCoord> Ghost::path_for_ghost(GhostId ghost_id) {
    static constexpr std::array pinky_path = { MapCoord{13, 13}, MapCoord{13, 12}, MapCoord{13, 11} };
    static constexpr std::array inky_path  = { MapCoord{11, 14}, MapCoord{12, 14}, MapCoord{13, 11} };
    static constexpr std::array clyde_path = {
        MapCoord{15, 14},
        MapCoord{14, 14},
        MapCoord{13, 14},
        MapCoord{13, 13},
        MapCoord{13, 12},
        MapCoord{13, 11}
    };

    if (ghost_id == GhostId::Pinky) {
        return pinky_path;
    }
    if (ghost_id == GhostId::Inky) {
        return inky_path;
    }
    if (ghost_id == GhostId::Clyde) {
        return clyde_path;
    }

    // Blinky start at (13,11) so it is already outside and skip the exit step.
    return {};
}

void Ghost::move_toward(MapCoord target, float dt) {
    accumulator_ += speed_ * dt;

    while(accumulator_ >= 1.0f) {
        accumulator_ -= 1.0f;

        if (offset_ == 0) {
            int c = target.col - col_;
            int r = target.row - row_;

            current_dir_.x = c != 0 ? c / std::abs(c) : 0;
            current_dir_.y = r != 0 ? r / std::abs(r) : 0;
        }

        if (current_dir_.x == 0 && current_dir_.y == 0)
            break;  // no direction — don't advance offset

        // Advance one pixel in the current direction
        offset_ += 1;

        if (offset_ == TILE_SIZE) {
            // Completed crossing into the next tile
            col_ += current_dir_.x;
            row_ += current_dir_.y;
            offset_ = 0;
        }
    }
}

void Ghost::move_toward_greedy(MapCoord target, float dt) {
    accumulator_ += speed_ * dt;

    while(accumulator_ >= 1.0f) {
        accumulator_ -= 1.0f;

        if (offset_ == 0) {
            static constexpr std::array<Dir, 4> canonical_dirs = { { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } } };

            std::array<Dir, 4> best_dirs{};
            int best_count = 0;
            int min_dist = std::numeric_limits<int>::max();

            for (const Dir& dir : canonical_dirs) {
                if (!can_move(col_, row_, dir)) continue;
                int dx = (col_ + dir.x) - target.col;
                int dy = (row_ + dir.y) - target.row;
                int dist = dx * dx + dy * dy;
                if (dist < min_dist) {
                    min_dist = dist;
                    best_count = 0;
                }
                if (dist == min_dist)
                    best_dirs[best_count++] = dir;
            }

            if (best_count > 0)
                current_dir_ = best_dirs[random_int(0, best_count - 1)];
        }

        if (current_dir_.x == 0 && current_dir_.y == 0)
            break;  // no direction — don't advance offset

        // Advance one pixel in the current direction
        offset_ += 1;

        if (offset_ == TILE_SIZE) {
            // Completed crossing into the next tile
            col_ += current_dir_.x;
            row_ += current_dir_.y;
            offset_ = 0;
        }
    }
}

bool Ghost::ghost_reached(MapCoord target) {
    log_trace("ghost_reached " + std::to_string(col_) + "," + std::to_string(row_));
    return col_ == target.col && row_ == target.row;
}

AABB Ghost::get_bounds() const {
    constexpr float margin = 2.f;
    return { pixel_x() + margin, pixel_y() + margin, TILE_SIZE - margin * 2, TILE_SIZE - margin * 2};
}
