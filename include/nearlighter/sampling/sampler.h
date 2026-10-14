#ifndef NEARLIGHTER_SAMPLING_SAMPLER_H
#define NEARLIGHTER_SAMPLING_SAMPLER_H

#include <nearlighter/math/vec2.h>
#include <nearlighter/math/vec3.h>

#include <cstdint>

/**
 * Deterministic Pseudo-Random Stream
 *
 * Owns one PCG32 state and advances it whenever a sample is requested.
 * Explicit ownership isolates paths from scheduling and other paths' random draws.
 */
class Sampler {
public:
    /**
     * Stream Initialization
     *
     * `sequence` selects the stream; `seed` determines its initial state.
     * Equal argument pairs reproduce the same sample sequence.
     */
    explicit Sampler(std::uint64_t seed, std::uint64_t sequence = 1);

    /**
     * @name Raw Random Bits
     *
     * Advances once per 32-bit result.
     * A 64-bit result consumes two consecutive 32-bit results.
     * @{
     */
    std::uint32_t nextUInt32();
    std::uint64_t nextUInt64();
    /** @} */

    /**
     * @name Uniform Scalar Samples
     *
     * Floating-point ranges are half-open.
     * Integer ranges include both bounds.
     *
     * Reversed ranges throw `std::invalid_argument`.
     * `nextInt()` also rejects ranges beyond the number of distinct uint32_t values.
     * @{
     */
    float next1D();
    float next1D(float min, float max);
    /** Returns two consecutive independent samples in `[0, 1)`. */
    Vec2f next2D();
    int nextInt(int min, int max);
    /** @} */

    /**
     * @name Vector and Direction Samples
     * Vector components use independent uniform samples.
     * The sphere method returns a unit direction; the disk method returns a point.
     * @{
     */
    Vec3f nextVec3();
    Vec3f nextVec3(float min, float max);
    /** Uniform unit-sphere direction. */
    Vec3f nextUnitVector();
    /** Uniform point in the open unit disk on the xy plane. */
    Vec3f nextInUnitDisk();
    /** @} */

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 0;
};

/**
 * Path Seed Derivation
 *
 * Hashes the render seed, pixel coordinates, and sample index into one stream seed.
 * The result is independent of traversal order and thread assignment.
 */
std::uint64_t derivePathSeed(std::uint64_t render_seed,
                             std::uint32_t pixel_x,
                             std::uint32_t pixel_y,
                             std::uint32_t sample_index);

#endif  // NEARLIGHTER_SAMPLING_SAMPLER_H
