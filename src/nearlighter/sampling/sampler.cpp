#include <nearlighter/sampling/sampler.h>

#include <nearlighter/math/constants.h>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

constexpr std::uint64_t kPcgMultiplier = 6364136223846793005ULL;
constexpr float kFloatScale = 1.0f / 16777216.0f;  // 2^-24

std::uint64_t mixBits(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

}  // namespace

Sampler::Sampler(std::uint64_t seed, std::uint64_t sequence)
    : increment_((sequence << 1U) | 1U) {
    /* PCG's two-step initialization incorporates both stream and seed. */
    nextUInt32();
    state_ += seed;
    nextUInt32();
}

std::uint32_t Sampler::nextUInt32() {
    const std::uint64_t old_state = state_;
    state_ = old_state * kPcgMultiplier + increment_;

    const std::uint32_t xorshifted = static_cast<std::uint32_t>(
        ((old_state >> 18U) ^ old_state) >> 27U);
    const std::uint32_t rotation = static_cast<std::uint32_t>(old_state >> 59U);
    return (xorshifted >> rotation) |
           (xorshifted << ((-rotation) & 31U));
}

std::uint64_t Sampler::nextUInt64() {
    return (static_cast<std::uint64_t>(nextUInt32()) << 32U) |
           static_cast<std::uint64_t>(nextUInt32());
}

float Sampler::next1D() {
    // Retaining the most significant 24 bits matches float's exact mantissa.
    return static_cast<float>(nextUInt32() >> 8U) * kFloatScale;
}

float Sampler::next1D(float min, float max) {
    if (max < min) {
        throw std::invalid_argument("Sampler range maximum must not be below minimum");
    }
    return min + (max - min) * next1D();
}

int Sampler::nextInt(int min, int max) {
    if (max < min) {
        throw std::invalid_argument("Sampler integer maximum must not be below minimum");
    }

    const std::uint64_t span64 =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(max) -
                                   static_cast<std::int64_t>(min)) +
        1ULL;
    if (span64 > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Sampler integer range is too large");
    }

    const std::uint32_t span = static_cast<std::uint32_t>(span64);
    const std::uint32_t threshold = static_cast<std::uint32_t>(-span) % span;
    std::uint32_t value;
    do {
        value = nextUInt32();
    } while (value < threshold);

    return min + static_cast<int>(value % span);
}

Vec3f Sampler::nextVec3() {
    return Vec3f(next1D(), next1D(), next1D());
}

Vec3f Sampler::nextVec3(float min, float max) {
    return Vec3f(next1D(min, max), next1D(min, max), next1D(min, max));
}

Vec3f Sampler::nextUnitVector() {
    /* Rejection sampling avoids a singularity from normalizing zero. */
    while (true) {
        const Vec3f point = nextVec3(-1.0f, 1.0f);
        const float length_squared = point.length_squared();
        if (length_squared > 1e-12f && length_squared <= 1.0f) {
            return point / std::sqrt(length_squared);
        }
    }
}

Vec3f Sampler::nextInUnitDisk() {
    while (true) {
        const Vec3f point(next1D(-1.0f, 1.0f),
                          next1D(-1.0f, 1.0f), 0.0f);
        if (point.length_squared() < 1.0f) return point;
    }
}

Vec3f Sampler::nextCosineHemisphere() {
    const float r1 = next1D();
    const float r2 = next1D();
    const float phi = 2.0f * pi * r1;
    const float sin_theta = std::sqrt(r2);

    return Vec3f(sin_theta * std::cos(phi),
                 sin_theta * std::sin(phi),
                 std::sqrt(1.0f - r2));
}

std::uint64_t derivePathSeed(std::uint64_t render_seed,
                             std::uint32_t pixel_x,
                             std::uint32_t pixel_y,
                             std::uint32_t sample_index) {
    std::uint64_t value = mixBits(render_seed);
    value = mixBits(value ^ (static_cast<std::uint64_t>(pixel_x) << 32U) ^
                    static_cast<std::uint64_t>(pixel_y));
    return mixBits(value ^ static_cast<std::uint64_t>(sample_index));
}
