#ifndef PERLIN_H
#define PERLIN_H

#include <nearlighter/math/vec3f.h>

#include <cstdint>

class Sampler;

class PerlinGenerator {
public:
    explicit PerlinGenerator(std::uint64_t seed = 0);

    float noise(const Point3f& p) const;
    float turbulence(const Point3f& p, int depth = 7) const;

private:
    static const int point_cnt = 256;
    Vec3f randvec[point_cnt];  // float randfloat[point_cnt];
    int perm_x[point_cnt];
    int perm_y[point_cnt];
    int perm_z[point_cnt]; // TODO: whether need another Vec3i

    static void generatePermutations(int* p, Sampler& sampler);
    static void permute(int* p, int n, Sampler& sampler);
    static float perlin_interpolate(const Vec3f c[2][2][2], float u, float v, float w);
};


#endif // PERLIN_H
