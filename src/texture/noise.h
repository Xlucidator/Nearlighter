#ifndef NOISE_H
#define NOISE_H

#include "texture.h"
#include "utils/perlin.h"

class NoiseTexture : public Texture {
public:
    NoiseTexture() {}
    NoiseTexture(float scale) : scale(scale) {}

    Color value(float u, float v, const Point3f& p) const override {
        return 0.5f * (1.0f + perlin.noise(scale * p)) * Color(1, 1, 1);
    }

private:
    PerlinGenerator perlin;
    float scale = 1.0f;
};

#endif // NOISE_H
