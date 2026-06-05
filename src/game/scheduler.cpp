
module game.scheduler;

void Scheduler::update(float dt) {
    for (auto& fn : updatables_)
        fn(dt);
}
