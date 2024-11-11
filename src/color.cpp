#include "color.h"

#include <iostream>

void write_color(std::ostream& out, const Color& pixel_color) {
    auto r = pixel_color.x();  // use auto: float or double
    auto g = pixel_color.y();  // value: [0,1]
    auto b = pixel_color.z();

    // Translate [0, 1] to [0, 255]
    int r_byte = int(r * 255.999);
    int g_byte = int(g * 255.999);
    int b_byte = int(b * 255.999);

    out << r_byte << ' ' << g_byte << ' ' << b_byte << ' ' << std::endl;
}