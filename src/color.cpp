#include "color.h"

#include <iostream>

bool impl_gamma_correct = false;
const int LUT_SIZE = 256;
uint8_t gamma_lut[LUT_SIZE];

void initGammaLUT(float gamma) {
    impl_gamma_correct = true;
    float inv_gamma = 1.0f / gamma;
    for (int i = 0; i < LUT_SIZE; ++i) {
        float corrected = std::pow(i / 255.0f, inv_gamma);
        gamma_lut[i] = static_cast<uint8_t>(corrected * 255.0f);
    }
}

void writeColor(std::ostream& out, const Color& pixel_color) {
    auto r = pixel_color.x();  // use auto: float or double
    auto g = pixel_color.y();  // value: [0,1]
    auto b = pixel_color.z();

    // Translate [0, 1] to [0, 255]
    static const Interval intensity(0.000f, 0.999f);
    int r_byte = int(intensity.clamp(r) * 256);
    int g_byte = int(intensity.clamp(g) * 256);
    int b_byte = int(intensity.clamp(b) * 256);

    if (impl_gamma_correct) {
        r_byte = gamma_lut[r_byte];
        g_byte = gamma_lut[g_byte];
        b_byte = gamma_lut[b_byte];
    }

    out << r_byte << ' ' << g_byte << ' ' << b_byte << ' ' << std::endl;
}