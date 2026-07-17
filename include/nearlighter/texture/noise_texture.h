#ifndef NOISE_H
#define NOISE_H

#include <nearlighter/texture/texture.h>
#include <nearlighter/texture/perlin.h>

#include <cstdint>

class NoiseTexture : public Texture {
public:
    NoiseTexture();
    explicit NoiseTexture(float scale, std::uint64_t seed = 0);

    Color value(float u, float v, const Point3f& p) const override;

private:
    PerlinGenerator perlin;
    float scale = 1.0f;
};

#endif // NOISE_H
