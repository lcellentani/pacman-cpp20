export module game.ghost;

import engine.types;
import game.scheduler;
import game.map;
import game.types;

export class Ghost {
public:
    explicit Ghost(GhostId id);

    void reset(Scheduler& scheduler, const Map* map);

    [[nodiscard]] GhostDebugState debug_state() const;

    Color get_color() const;

private:
    friend struct move_to;

    GhostId id_;
    MapCoord target_;

    Task behavior_;

    const Map* map_ = nullptr; // pointer, rebindable, nullable

    Task wander(Scheduler& scheduler);

    MapCoord pick_random_target();
    void move_toward(MapCoord target, float dt);
    bool ghost_reached(MapCoord target);
};
