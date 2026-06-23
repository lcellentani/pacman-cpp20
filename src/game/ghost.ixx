module;
#include <span>
#include <vector>

export module game.ghost;

import engine.renderer;
import engine.types;
import game.scheduler;
import game.map;
import game.types;

export class Ghost {
public:
    explicit Ghost(GhostId id);

    void reset(Scheduler& scheduler, const Map* map, GameState* game_state);
    
    MapCoord current_coord() const { return { col_, row_ }; }

    void draw(Renderer& renderer);
    
    [[nodiscard]] Color get_color() const;
    [[nodiscard]] AABB get_bounds() const;

     [[nodiscard]] GhostDebugState debug_state() const;

private:
    friend struct move_to;
    friend struct walk_path;
    friend struct wait_for_release;

    GhostId id_;
    MapCoord target_;

    int row_ = 0;
    int col_ = 0;
    int offset_ = 0;

    Dir current_dir_ { 0, 0 };

    GhostState state_ = GhostState::Scatter; // actual current phase
    bool phase_started_ = false;             // suppress reversal on the very first phase
    bool reverse_pending_ = false;           // set on mode switch, consumed at next tile center

    float accumulator_ = 0.f;
    float speed_ = 0.0f;

    const Map* map_ = nullptr; // pointer, rebindable, nullable
    GameState* game_state_ = nullptr;

    std::vector<MapCoord> path_;
    Task behavior_;

    Task behavior(Scheduler& scheduler);
    Task release_from_house(Scheduler& scheduler, int dot_threshold, float force_release_seconds);
    void enter_phase(GhostState s);

    int pixel_x() const;
    int pixel_y() const;

    bool can_move(int col, int row, Dir dir) const;

    MapCoord pick_chase_target_for_ghost(GhostId ghost_id) const;
    MapCoord pick_random_target();
    std::span<const MapCoord> path_for_ghost(GhostId ghost_id);

    void move_toward(MapCoord target, float dt);
    void move_toward_greedy(MapCoord target, float dt);
    bool ghost_reached(MapCoord target);
};
