#include "utils.h"

#include <limits>

const float infinity = std::numeric_limits<float>::infinity();
const float pi = acos(-1);

#ifdef RANDOM_NEW

std::mt19937 gen(std::random_device{}());

#endif