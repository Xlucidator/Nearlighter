#include "perlin.h"

PerlinGenerator::PerlinGenerator() {
    for (int i = 0; i < point_cnt; i++) randfloat[i] = random_float();
    generate_permutations(perm_x);
    generate_permutations(perm_y);
    generate_permutations(perm_z);
}

float PerlinGenerator::noise(const Point3f& p) const {
    // need to add std::floor(), or there will be a explicit line crossing the axis
    //      for static_cast in c++ was 'truncate': int(1.5) = 1 and int(-1.5) = -1
    int i = static_cast<int>(std::floor(p.x()));
    int j = static_cast<int>(std::floor(p.y()));
    int k = static_cast<int>(std::floor(p.z()));

    // offset to the unit cube
    float u = p.x() - static_cast<float>(i);
    float v = p.y() - static_cast<float>(j);
    float w = p.z() - static_cast<float>(k);

    // get the random values of a 2x2x2 cube
    float c[2][2][2];
    for (int di = 0; di < 2; di++)
        for (int dj = 0; dj < 2; dj++)
            for (int dk = 0; dk < 2; dk++)
                c[di][dj][dk] = randfloat[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^ perm_z[(k + dk) & 255]];

    return trilinear_interpolate(c, u, v, w);
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

/* Trilinear Interpolation
 *  simply extend from bilinear interpolation : bilinear interpolation is one layer of trilinear interpolation
 *  => x axis:
 *      linear_interpolate[0][0] = u * c[0][0][0] + (1 - u) * c[0][0][1]
 *      linear_interpolate[0][1] = u * c[0][1][0] + (1 - u) * c[0][1][1]
 *      ---
 *      linear_interpolate[1][0] = u * c[1][0][0] + (1 - u) * c[1][0][1]
 *      linear_interpolate[1][1] = u * c[1][1][0] + (1 - u) * c[1][1][1]
 *  => y axis:
 *      bilinear_interpolate[0] = v * linear_interpolate[0][0] + (1 - v) * linear_interpolate[0][1]
 *                              = c[0][0][0] * u * v + c[0][0][1] * u * (1 - v) + c[0][1][0] * (1 - u) * v + c[0][1][1] * (1 - u) * (1 - v)
 *      bilinear_interpolate[1] = v * linear_interpolate[1][0] + (1 - v) * linear_interpolate[1][1]
 *                              = c[1][0][0] * u * v + c[1][0][1] * u * (1 - v) + c[1][1][0] * (1 - u) * v + c[1][1][1] * (1 - u) * (1 - v)
 *  => z axis:
 *      trilinear_interpolate = w * bilinear_interpolate[0] + (1 - w) * bilinear_interpolate[1]
 *                              = c[0][0][0] * u * v * w + c[0][0][1] * u * v * (1 - w) + c[0][1][0] * u * (1 - v) * w + c[0][1][1] * u * (1 - v) * (1 - w)
 *                              + c[1][0][0] * (1 - u) * v * w + c[1][0][1] * (1 - u) * v * (1 - w) + c[1][1][0] * (1 - u) * (1 - v) * w + c[1][1][1] * (1 - u) * (1 - v) * (1 - w)
 */
float PerlinGenerator::trilinear_interpolate(const float c[2][2][2], float u, float v, float w) {
    float res = 0.0f;
    for (int i = 0; i < 2; ++i) 
        for (int j = 0; j < 2; ++j)
            for (int k = 0; k < 2; ++k)
                res += (i * u + (1 - i) * (1 - u)) *
                       (j * v + (1 - j) * (1 - v)) *
                       (k * w + (1 - k) * (1 - w)) * c[i][j][k];
    return res;
}