#include <nearlighter/texture/noise_texture.h>

#include <cmath>

NoiseTexture::NoiseTexture() {}

NoiseTexture::NoiseTexture(float scale)
    : scale(scale) {}

Color NoiseTexture::value(float, float, const Point3f& p) const {
    // Marble-like texture: turbulence controls the phase of the sine wave.
    return 0.5f * (1 + std::sin(scale * p.z() + 10 * perlin.turbulence(p, 7))) * Color(1, 1, 1);
}
