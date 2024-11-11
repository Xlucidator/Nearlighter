#ifndef COLOR_H
#define COLOR_H

#include "vec3f.h"

typedef Vec3f Color;

void write_color(std::ostream& out, const Color& pixel_color);

#endif
