module;
#include <random>

module engine.random;

Random::Random() : engine_(std::random_device{}()) {}

Random& Random::instance() {
    static Random inst;
    return inst;
}

int Random::int_in(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(engine_);
}

float Random::float_in(float lo, float hi) {
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(engine_);
}

void Random::seed(unsigned s) {
    engine_.seed(s);
}

int   random_int  (int lo, int hi)     { return Random::instance().int_in(lo, hi); }
float random_float(float lo, float hi) { return Random::instance().float_in(lo, hi); }
