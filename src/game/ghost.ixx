export module game.ghost;

import engine.renderer;
import engine.types;
import game.scheduler;
import game.map;
import game.types;

export class Ghost {
public:
    explicit Ghost(GhostId id);

    void reset(Scheduler& scheduler, const Map* map);
    
    [[nodiscard]] GhostDebugState debug_state() const;

    void draw(Renderer& renderer);
    Color get_color() const;

    AABB get_bounds() const;

private:
    friend struct move_to;

    GhostId id_;
    MapCoord target_;

    int row_ = 0;
    int col_ = 0;
    int offset_ = 0;

    Dir current_dir_ {0, 0};

    float accumulator_ = 0.f;
    float speed_ = 0.0f;

    const Map* map_ = nullptr; // pointer, rebindable, nullable

    Task behavior_;

    Task wander(Scheduler& scheduler);

    int pixel_x() const;
    int pixel_y() const;

    bool can_move(int col, int row, Dir dir) const;

    MapCoord pick_random_target();
    void move_toward(MapCoord target, float dt);
    bool ghost_reached(MapCoord target);
};
