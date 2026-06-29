#ifndef NOISE_H
#define NOISE_H

#include <nearlighter/texture/texture.h>
#include <nearlighter/texture/perlin.h>

class NoiseTexture : public Texture {
public:
    NoiseTexture();
    NoiseTexture(float scale);

    Color value(float u, float v, const Point3f& p) const override;

private:
    PerlinGenerator perlin;
    float scale = 1.0f;
};

#endif // NOISE_H
