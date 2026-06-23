module;
#include <array>
#include <coroutine>
#include <exception>
#include <optional>
#include <span>

export module game.types;

import engine.types;

export constexpr int MAP_COLS = 28;
export constexpr int MAP_ROWS = 31;
export constexpr int TILE_SIZE = 24;
export constexpr int HALF_TILE_SIZE = TILE_SIZE / 2; // 12
export constexpr int WINDOW_W = MAP_COLS * TILE_SIZE;   // 672
export constexpr int WINDOW_H = MAP_ROWS * TILE_SIZE;   // 744
export constexpr int DEBUG_PANEL_W = 430;

// Fixed-layout panel positions / sizes for the ImGui workbench.
// Game panel hosts an ImGui::Image of the off-screen game texture.
export constexpr int LAYOUT_GAME_X    = 10;
export constexpr int LAYOUT_GAME_Y    = 10;
export constexpr int LAYOUT_DEBUG_X   = LAYOUT_GAME_X + WINDOW_W + 40;
export constexpr int LAYOUT_DEBUG_Y   = 10;
export constexpr int LAYOUT_DEBUG_W   = DEBUG_PANEL_W;
export constexpr int LAYOUT_DEBUG_H   = WINDOW_H;
export constexpr int LAYOUT_CONSOLE_X = LAYOUT_GAME_X;
export constexpr int LAYOUT_CONSOLE_Y = LAYOUT_GAME_Y + WINDOW_H + 60;
export constexpr int LAYOUT_CONSOLE_W = WINDOW_W + DEBUG_PANEL_W + 30;
export constexpr int LAYOUT_CONSOLE_H = 260;
export constexpr int LAYOUT_WINDOW_W  = LAYOUT_CONSOLE_X + LAYOUT_CONSOLE_W + 20;
export constexpr int LAYOUT_WINDOW_H  = LAYOUT_CONSOLE_Y + LAYOUT_CONSOLE_H + 20;

export struct MapCoord {
    int col = 0;
    int row = 0;

    friend bool operator==(const MapCoord&, const MapCoord&) = default;
};

export struct PacmanDebugState {
    MapCoord coord;
    int dir_x, dir_y;
    float speed;
    AABB  bounds;
};

export struct Dir { int x, y; }; // values always in {-1, 0, 1}

// Order encodes the arcade ghost tie-break priority: Up > Left > Down > Right.
// Consumers that break ties by "first acceptable direction" rely on this order.
export [[nodiscard]] inline std::span<const Dir> cardinal_dirs() {
    static constexpr std::array<Dir, 4> dirs = { { { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, 0 } } };
    return dirs;
}

export enum class GhostState { House, Scatter, Chase, Frightened, Dead };

export enum class GhostId { Blinky, Pinky, Inky, Clyde };

export [[nodiscard]] inline std::span<const GhostId> ghost_release_order() {
    static constexpr std::array<GhostId, 3> order = { { GhostId::Pinky, GhostId::Inky, GhostId::Clyde } };
    return order;
}

export struct GhostDebugState {
    GhostId    id;
    MapCoord   coord;
    int        dir_x, dir_y;
    float      speed;
    AABB       bounds;
    Color      color;
    GhostState state;
    MapCoord   target; // {-1,-1} when not applicable (Frightened/Dead)
    std::span<const MapCoord> path;
};

export struct GameState {
    MapCoord pacman_tile;
    Dir pacman_dir;

    MapCoord blinky_tile;

    int dots_eaten;
    float dots_timer;

    std::optional<GhostId> next_force_release;
    size_t next_ghost_release_index;
};

export struct Task {
    struct promise_type {
        std::coroutine_handle<> continuation_;

        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() { return {}; }

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                if (handle.promise().continuation_) {
                    handle.promise().continuation_.resume();
                }
            }
            void await_resume() noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle_;

    explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
    ~Task() {
        if (handle_) handle_.destroy();
    }

    Task() : handle_(nullptr) {}
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) : handle_(other.handle_) { other.handle_ = nullptr; }
    Task& operator=(Task&& other) {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = other.handle_;
            other.handle_ = {};
        }
        return *this;
    }

    void resume() {
        handle_.resume();
    }

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> continuation) {
        handle_.promise().continuation_ = continuation;
        handle_.resume();
    }
    void await_resume() {}
};