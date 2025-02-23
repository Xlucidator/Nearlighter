#ifndef INTERVAL_H
#define INTERVAL_H

#include "utils/constant.h"

#include <iostream>

class Interval {
public:
    float min, max;
    static const Interval empty, universe;

    Interval() : min(+infinity), max(-infinity) {} // Default interval is empty
    Interval(float min, float max) : min(min), max(max) {}
    Interval(const Interval& i0, const Interval& i1) : min(std::min(i0.min, i1.min)), max(std::max(i0.max, i1.max)) {}

    /* Modifiable Calculation */
    void uunion(const Interval& other) { min = std::min(min, other.min), max = std::max(max, other.max); }

    /* Property Checking */
    // Self
    float size() const { return max - min; }
    float center() const { return (min + max) * 0.5; }
    bool is_null() const { return max <= min; }
    // Out
    bool contains(float x) const { return min <= x && x <= max; }
    bool surrounds(float x) const { return min < x && x < max; }
    float clamp(float x) const;
    Interval expand(float delta) const;

    /* Debug */
    friend std::ostream& operator<<(std::ostream& os, const Interval& itv) {
        return os << "[" << itv.min << ", " << itv.max << "]";
    }
    std::string toString() const { return "[" + std::to_string(min) + ", " + std::to_string(max) + "]"; }
};

#endif
