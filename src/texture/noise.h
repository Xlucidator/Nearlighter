#ifndef NOISE_H
#define NOISE_H

#include "texture.h"
#include "utils/perlin.h"

class NoiseTexture : public Texture {
public:
    NoiseTexture() {}
    NoiseTexture(float scale) : scale(scale) {}

    Color value(float u, float v, const Point3f& p) const override {
        // Marble-like texture: Use turbulence to control the phase of the sine wave
        return 0.5f * (1 + std::sin(scale * p.z() + 10 * perlin.turbulence(p, 7))) * Color(1, 1, 1);
        // Simple perlin noise
        return 0.5f * (1.0f + perlin.noise(scale * p)) * Color(1, 1, 1);
    }

private:
    PerlinGenerator perlin;
    float scale = 1.0f;
};

#endif // NOISE_H
