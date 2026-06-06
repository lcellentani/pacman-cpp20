module;
#include <random>

export module engine.random;

export class Random {
public:
    static Random& instance();

    // Inclusive on both ends.
    int   int_in(int lo, int hi);
    float float_in(float lo, float hi);

    void seed(unsigned s);

private:
    Random();

    std::mt19937 engine_;
};

export int   random_int  (int lo, int hi);
export float random_float(float lo, float hi);
