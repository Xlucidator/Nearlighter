#ifndef COLOR_H
#define COLOR_H

#include <nearlighter/math/vec3f.h>
#include <nearlighter/base/interval.h>

typedef Vec3f Color;

extern bool impl_gamma_correct;
extern const int LUT_SIZE;
extern uint8_t gamma_lut[];

void initGammaLUT(float gamma = 2.2f);

void writeColor(std::ostream& out, const Color& pixel_color);

#endif
