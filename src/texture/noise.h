#ifndef NOISE_H
#define NOISE_H

#include "texture.h"
#include "utils/perlin.h"

class NoiseTexture : public Texture {
public:
    NoiseTexture() {}
    NoiseTexture(float scale) : scale(scale) {}

    Color value(float u, float v, const Point3f& p) const override {
        return Color(1, 1, 1) * perlin.noise(scale * p);
    }

private:
    PerlinGenerator perlin;
    float scale = 1.0f;
};

#endif // NOISE_H
