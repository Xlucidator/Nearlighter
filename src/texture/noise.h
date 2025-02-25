#ifndef NOISE_H
#define NOISE_H

#include "texture.h"
#include "utils/perlin.h"

class NoiseTexture : public Texture {
public:
    NoiseTexture() {}

    Color value(float u, float v, const Point3f& p) const override {
        return Color(1, 1, 1) * perlin.noise(p);
    }

private:
    PerlinGenerator perlin;
};

#endif // NOISE_H
