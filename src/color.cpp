#include "color.h"

#include <iostream>

void write_color(std::ostream& out, const Color& pixel_color) {
    auto r = pixel_color.x();  // use auto: float or double
    auto g = pixel_color.y();  // value: [0,1]
    auto b = pixel_color.z();

    // Translate [0, 1] to [0, 255]
    static const Interval intensity(0.000f, 0.999f);
    int r_byte = int(intensity.clamp(r) * 256);
    int g_byte = int(intensity.clamp(g) * 256);
    int b_byte = int(intensity.clamp(b) * 256);

    out << r_byte << ' ' << g_byte << ' ' << b_byte << ' ' << std::endl;
}