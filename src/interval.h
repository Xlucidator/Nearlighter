#ifndef INTERVAL_H
#define INTERVAL_H

#include "utils.h"

class Interval {
public:
    float min, max;

    Interval() : min(+infinity), max(-infinity) {} // Default interval is empty

    Interval(float min, float max) : min(min), max(max) {}

    float size() const { return max - min; }
    bool contains(float x) const { return min <= x && x <= max; }
    bool surrounds(float x) const { return min < x && x < max; }
    float clamp(float x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const Interval empty, universe;
};

#endif
