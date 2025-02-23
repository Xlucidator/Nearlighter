#ifndef RANDOM_H
#define RANDOM_H

#define RANDOM_NEW

#ifdef RANDOM_NEW

#include <random>

// class Random {
// public:
//     static void init() { set_seed(std::random_device{}()); }
//     static void init(unsigned int seed) { set_seed(seed); }
//     static void set_seed(unsigned int seed) { generator = std::mt19937(seed); }

//     static float random_float() {
//         static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
//         return dis(generator);
//     }

//     static float random_float(float min, float max) {
//         static std::uniform_real_distribution<float> dis;
//         dis.param(std::uniform_real_distribution<float>::param_type(min, max));
//         return dis(generator);
//     }

// private:
//     static std::mt19937 generator;
// };

inline float random_float() {
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    static std::mt19937 gen(std::random_device{}());
    return dis(gen);
}

inline float random_float(float min, float max) {
    return min + (max - min) * random_float();
}


#else

#include <cstdlib>

inline float random_float() {
    return std::rand() / (RAND_MAX + 1.0);
}

inline float random_float(float min, float max) {
    return min + (max - min) * random_float();
}

#endif

#endif // RANDOM_H