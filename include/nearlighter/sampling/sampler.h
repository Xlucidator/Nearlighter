#ifndef NEARLIGHTER_SAMPLING_SAMPLER_H
#define NEARLIGHTER_SAMPLING_SAMPLER_H

#include <nearlighter/math/vec3f.h>

#include <cstdint>

/**
 * Owns one deterministic pseudo-random sample sequence.
 *
 * A Sampler is intentionally small and passed explicitly through stochastic
 * rendering operations. This makes random-number ownership visible and keeps
 * a path's result independent of thread scheduling or other pixels.
 */
class Sampler {
public:
    /** Starts a deterministic PCG stream from a seed and stream selector. */
    explicit Sampler(std::uint64_t seed, std::uint64_t sequence = 1);

    /** Returns the next raw 32-bit PCG value. */
    std::uint32_t nextUInt32();

    /** Returns the next raw 64-bit value assembled from two PCG values. */
    std::uint64_t nextUInt64();

    /** Returns a uniformly distributed value in `[0, 1)`. */
    float next1D();

    /** Returns a uniformly distributed value in `[min, max)`. */
    float next1D(float min, float max);

    /** Returns an unbiased uniformly distributed integer in `[min, max]`. */
    int nextInt(int min, int max);

    /** Returns a vector whose components are uniformly sampled in `[0, 1)`. */
    Vec3f nextVec3();

    /** Returns a vector whose components are uniformly sampled in `[min, max)`. */
    Vec3f nextVec3(float min, float max);

    /** Returns a uniformly distributed direction on the unit sphere. */
    Vec3f nextUnitVector();

    /** Returns a uniformly distributed point inside the unit disk. */
    Vec3f nextInUnitDisk();

    /** Returns a cosine-weighted direction around the positive z axis. */
    Vec3f nextCosineHemisphere();

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 0;
};

/**
 * Derives the deterministic random sequence seed for one pixel sample.
 *
 * The result depends only on the render seed, pixel coordinates, and sample
 * index, so traversal order and thread assignment do not affect the image.
 */
std::uint64_t derivePathSeed(std::uint64_t render_seed,
                             std::uint32_t pixel_x,
                             std::uint32_t pixel_y,
                             std::uint32_t sample_index);

#endif  // NEARLIGHTER_SAMPLING_SAMPLER_H
