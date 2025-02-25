#ifndef PERLIN_H
#define PERLIN_H

#include "vec3f.h"
#include "utils/random.h"

class PerlinGenerator {
public:
    PerlinGenerator();

    float noise(const Point3f& p) const;
    float turbulence(const Point3f& p, int depth = 7) const;

private:
    static const int point_cnt = 256;
    float randfloat[point_cnt];
    int perm_x[point_cnt];
    int perm_y[point_cnt];
    int perm_z[point_cnt]; // TODO: whether need another Vec3i

    static void generate_permutations(int* p);
    static void permute(int* p, int n);
    static float trilinear_interpolate(const float c[2][2][2], float u, float v, float w);
};


#endif // PERLIN_H
