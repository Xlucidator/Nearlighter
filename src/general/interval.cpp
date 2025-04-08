
#include "interval.h"

const Interval Interval::empty    = Interval(+infinity, -infinity);
const Interval Interval::universe = Interval(-infinity, +infinity);
const Interval Interval::unit     = Interval(0, 1);

float Interval::clamp(float x) const {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

Interval Interval::expand(float delta) const { 
    float padding = delta * 0.5;
    return Interval(min - padding, max + padding); 
}