module;
#include <array>
#include <cassert>
#include <coroutine>
#include <limits>
#include <optional>
#include <span>
#include <string>

module game.ghost;

import engine.log;
import engine.random;
import game.concepts;
import game.map;
import game.scheduler;
import game.types;

template<FrameCallback F>
struct wait_for {
    std::optional<float> timer_ = 0.0f;
    Scheduler& scheduler_;
    F callback_;
    
    Scheduler::Handle registration_ = 0;
    std::coroutine_handle<> handle_;

    wait_for(std::optional<float> delay, Scheduler& scheduler, F callback) :
        timer_(delay), scheduler_(scheduler), callback_(callback) {}
    
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        registration_ = scheduler_.register_updatable(*this);
    }

    void await_resume() {
        scheduler_.unregister_updatable(registration_);
    }

    void update(float dt) {
        callback_(dt);

        if (!timer_) return;  // indefinite — never resumes on its own

        *timer_ -= dt;
        if (timer_ <= 0.f) {
            handle_.resume();
        }
    }
};

struct wait_for_release {
    Ghost& ghost_;
    Scheduler& scheduler_;
    GameState& game_state_;
    int dot_threshold_;
    float force_release_seconds_;

    Scheduler::Handle registration_ = 0;
    std::coroutine_handle<> handle_;

    wait_for_release(Ghost& ghost, Scheduler& scheduler, GameState& game_state, int dot_threshold, float force_release_seconds)
        : ghost_(ghost)
        , scheduler_(scheduler)
        , game_state_(game_state)
        , dot_threshold_(dot_threshold)
        , force_release_seconds_(force_release_seconds) {}

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        handle_ = handle;
        registration_ = scheduler_.register_updatable(*this);
    }

    void await_resume() {
        scheduler_.unregister_updatable(registration_);
    }

    void update(float dt) {
        (void)dt; // unused

        bool advance_release_queue = false;
        if (game_state_.dots_eaten >= dot_threshold_) {
            advance_release_queue = true;
        }
        else if (game_state_.next_force_release == ghost_.id_ && game_state_.dot_timer > force_release_seconds_) {
            advance_release_queue = true;
            game_state_.dot_timer = 0.0f;
        }
        if (advance_release_queue) {
            game_state_.next_ghost_release_index++;
            game_state_.next_force_release = std::nullopt;
            if (game_state_.next_ghost_release_index < ghost_release_order().size()) {
                game_state_.next_force_release = ghost_release_order()[game_state_.next_ghost_release_index];
            }
            handle_.resume();
        }
    }
};

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

constexpr MapCoord scatter_target_for(GhostId ghost_id) {
    switch (ghost_id) {
    case GhostId::Blinky: return { 25, 0 };  // top-right corner
    case GhostId::Pinky:  return { 2, 0 };   // top-left corner
    case GhostId::Inky:   return { 27, 30 }; // bottom-right corner
    case GhostId::Clyde:  return { 0, 30 };  // bottom-left corner
    }
    return { 0, 0 };
}

constexpr int distance_squared(MapCoord a, MapCoord b) {
    int dx = a.col - b.col;
    int dy = a.row - b.row;
    return dx * dx + dy * dy;
}

// Arcade "n tiles ahead of Pac-Man" lookup. Reproduces the original ROM
// overflow bug: when Pac-Man faces up, the column is also shifted by -n.
constexpr MapCoord arcade_ahead(MapCoord tile, Dir dir, int n) {
    int col = tile.col + dir.x * n;
    int row = tile.row + dir.y * n;
    if (dir.x == 0 && dir.y == -1) col -= n;   // up: overflow bug
    return { col, row };
}

constexpr MapCoord inky_target(MapCoord pac, Dir pac_dir, MapCoord blinky) {
    const MapCoord pivot = arcade_ahead(pac, pac_dir, 2);
    const int vx = pivot.col - blinky.col;
    const int vy = pivot.row - blinky.row;
    return { blinky.col + 2 * vx, blinky.row + 2 * vy };
}

Ghost::Ghost(GhostId id) : id_(id) {}

void Ghost::reset(Scheduler& scheduler, const Map* map, GameState* game_state, int dot_threshold, float force_release_seconds) {
    map_ = map;
    game_state_ = game_state;

    dot_threshold_ = dot_threshold;
    force_release_seconds_ = force_release_seconds;

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
    return { id_, { col_, row_ }, current_dir_.x, current_dir_.y, speed_, get_bounds(), get_color(), state_, target_, path_ };
}

Task Ghost::behavior(Scheduler& scheduler) {
    static constexpr std::array<std::pair<float, float>, 3> phase_timings = {{
        { 7.0f, 20.0f },
        { 7.0f, 20.0f },
        { 5.0f, 20.0f }
    }};

    if (id_ != GhostId::Blinky) {
        co_await release_from_house(scheduler, dot_threshold_, force_release_seconds_);
    }

    while (true) {
        for(auto[scatter_time, chase_time] : phase_timings) {
            // scatter phase
            enter_phase(GhostState::Scatter);
            target_ = scatter_target_for(id_);
            co_await wait_for(scatter_time, scheduler, [this](float dt) {
                move_toward_greedy(target_, dt);
            });

            // chase phase
            enter_phase(GhostState::Chase);
            co_await wait_for(chase_time, scheduler, [this](float dt) {
                target_ = pick_chase_target_for_ghost(id_);
                move_toward_greedy(target_, dt);
            });
        }

        // scatter phase
        enter_phase(GhostState::Scatter);
        target_ = scatter_target_for(id_);
        co_await wait_for(5.0f, scheduler, [this](float dt) {
            move_toward_greedy(target_, dt);
        });

        // chase phase - indefinite
        enter_phase(GhostState::Chase);
        co_await wait_for(std::nullopt, scheduler, [this](float dt) {
            target_ = pick_chase_target_for_ghost(id_);
            move_toward_greedy(target_, dt);
        });
    }
}

Task Ghost::release_from_house(Scheduler& scheduler, int dot_threshold, float force_release_seconds) {
    state_ = GhostState::House;
    co_await wait_for_release(*this, scheduler, *game_state_, dot_threshold, force_release_seconds);
    co_await walk_path(*this, scheduler, path_for_ghost(id_));
}

void Ghost::enter_phase(GhostState s) {
    if (phase_started_ && s != state_)
        reverse_pending_ = true;   // arcade: reverse on every mode change after the first
    state_ = s;
    phase_started_ = true;
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

MapCoord Ghost::pick_chase_target_for_ghost(GhostId ghost_id) const {
   if ( ghost_id == GhostId::Blinky) {
        return game_state_->pacman_tile; // Blinky targets Pac-Man's current tile directly
    }
    if (ghost_id == GhostId::Pinky) {
        // Four tiles ahead of Pac-Man, reproducing the arcade up-overflow bug.
        return arcade_ahead(game_state_->pacman_tile, game_state_->pacman_dir, 4);
    }
    if (ghost_id == GhostId::Inky) {
        return inky_target(game_state_->pacman_tile, game_state_->pacman_dir, game_state_->blinky_tile);
    }
    if (ghost_id == GhostId::Clyde) {
        // Chase Pac-Man directly only when farther than 8 tiles (squared distance);
        // otherwise retreat to the scatter corner.
        constexpr int target_safety_distance_squared = 8 * 8;
        const int dist = distance_squared({ col_, row_ }, game_state_->pacman_tile);
        if (dist > target_safety_distance_squared) {
            return game_state_->pacman_tile;
        }
        return scatter_target_for(ghost_id);
    }
    return { 0, 0 }; 
}

MapCoord Ghost::pick_random_target() {
    return map_->pick_random_walkable();
}

std::span<const MapCoord> Ghost::path_for_ghost(GhostId ghost_id) {
    static constexpr std::array pinky_path = { MapCoord{13, 13}, MapCoord{13, 12}, MapCoord{13, 11} };
    static constexpr std::array inky_path  = { MapCoord{11, 14}, MapCoord{12, 14}, MapCoord{13, 11} };
    static constexpr std::array clyde_path = { MapCoord{15, 14}, MapCoord{14, 14}, MapCoord{13, 11} };

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

// Target refreshed every frame; move_toward_greedy consumes it
// at tile boundaries (offset_ == 0) per original arcade behaviour.
void Ghost::move_toward_greedy(MapCoord target, float dt) {
    accumulator_ += speed_ * dt;

    while(accumulator_ >= 1.0f) {
        accumulator_ -= 1.0f;

        if (offset_ == 0) {
            const Dir reverse_dir{ -current_dir_.x, -current_dir_.y };

            if (reverse_pending_) {
                // Mode switch (scatter<->chase) forces a direction reversal.
                reverse_pending_ = false;
                if ((reverse_dir.x != 0 || reverse_dir.y != 0) && can_move(col_, row_, reverse_dir))
                    current_dir_ = reverse_dir;
            }
            else {
                // Deterministic greedy choice. cardinal_dirs() is ordered by arcade
                // tie-break priority (Up > Left > Down > Right); strict '<' keeps the
                // first (highest-priority) direction at the minimum distance.
                Dir best_dir{ 0, 0 };
                int min_dist = std::numeric_limits<int>::max();
                bool found = false;

                for (const Dir& dir : cardinal_dirs()) {
                    if (dir.x == reverse_dir.x && dir.y == reverse_dir.y) continue;
                    if (!can_move(col_, row_, dir)) continue;
                    const int dist = distance_squared({ col_ + dir.x, row_ + dir.y }, target);
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_dir = dir;
                        found = true;
                    }
                }

                // Dead-end fallback: allow reversal only when no other walkable direction exists.
                if (!found && can_move(col_, row_, reverse_dir)) {
                    best_dir = reverse_dir;
                    found = true;
                }

                if (found)
                    current_dir_ = best_dir;
            }
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
    return col_ == target.col && row_ == target.row;
}

AABB Ghost::get_bounds() const {
    constexpr float margin = 2.f;
    return { pixel_x() + margin, pixel_y() + margin, TILE_SIZE - margin * 2, TILE_SIZE - margin * 2};
}
