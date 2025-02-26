#include "perlin.h"

#include "utils/math.h"

PerlinGenerator::PerlinGenerator() {
    for (int i = 0; i < point_cnt; i++) 
        randvec[i] = Vec3f::random_unit_vector();  // unit_vector(Vec3f::random(-1, 1))

    generate_permutations(perm_x);
    generate_permutations(perm_y);
    generate_permutations(perm_z);
}

/* Perlin Noise
 *  p: point in space
 *  => return value in [-1, 1]
 */
float PerlinGenerator::noise(const Point3f& p) const {
    // need to add std::floor(), or there will be a explicit line crossing the axis
    //      for static_cast in c++ was 'truncate': int(1.5) = 1 and int(-1.5) = -1
    int i = static_cast<int>(std::floor(p.x()));
    int j = static_cast<int>(std::floor(p.y()));
    int k = static_cast<int>(std::floor(p.z()));

    // offset to the unit cube
    float u = smoothstep(p.x() - i);
    float v = smoothstep(p.y() - j);
    float w = smoothstep(p.z() - k);

    // get the random values of a 2x2x2 cube
    Vec3f c[2][2][2];
    for (int di = 0; di < 2; di++)
        for (int dj = 0; dj < 2; dj++)
            for (int dk = 0; dk < 2; dk++)
                c[di][dj][dk] = randvec[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^ perm_z[(k + dk) & 255]];

    return perlin_interpolate(c, u, v, w);
}

float PerlinGenerator::turbulence(const Point3f& p, int depth) const {
    return 0.0;
}



/* Private */

void PerlinGenerator::generate_permutations(int* p) {
    for (int i = 0; i < point_cnt; ++i) p[i] = i;
    permute(p, point_cnt);
}

void PerlinGenerator::permute(int* p, int n) {
    int target;
    for (int i = n-1; i > 0; --i) {
        target = random_int(0, i);
        std::swap(p[i], p[target]);
    }
}

/* Perlin Interpolation
 *  grid value: float c[i]j][k] -> dot(Vec3f c[i][j][k], direction from (i, j, k) to (u, v, w))
 *  => direction = Vec3f(u, v, w) - Vec3f(i, j, k)
 */
float PerlinGenerator::perlin_interpolate(const Vec3f c[2][2][2], float u, float v, float w) {
    float res = 0.0f;

    float smooth_u = smoothstep(u);
    float smooth_v = smoothstep(v);
    float smooth_w = smoothstep(w);

    for (int i = 0; i < 2; ++i) 
        for (int j = 0; j < 2; ++j)
            for (int k = 0; k < 2; ++k)
                res += (i * smooth_u + (1 - i) * (1 - smooth_u)) *
                       (j * smooth_v + (1 - j) * (1 - smooth_v)) *
                       (k * smooth_w + (1 - k) * (1 - smooth_w)) * 
                       dot(c[i][j][k], Vec3f(u-i, v-j, w-k));
    return res;
}