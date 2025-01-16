#include "utils.h"

#include <limits>

const float infinity = std::numeric_limits<float>::infinity();
const float PI = acos(-1);

std::mt19937 gen(std::random_device{}());