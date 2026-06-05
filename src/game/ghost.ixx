module;
#include <utility>

export module game.ghost;

import game.scheduler;
import game.types;

export class Ghost {
public:
    void reset(Scheduler& scheduler);

private:
    friend struct move_to;

    Task behavior_;

    Task wander(Scheduler& scheduler);

    std::pair<int, int> pick_random_target();
    void move_toward(int col, int row, float dt);
    bool ghost_reached(int col, int row);
};
