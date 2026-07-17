#include "test_support.h"

#include <nearlighter/sampling/sampler.h>

#include <array>
#include <cstdint>

namespace {

void testSequenceReproducibility(nearlighter::test::Context& context) {
    Sampler first(42);
    Sampler second(42);
    Sampler different(43);
    bool different_sequence_found = false;

    for (int index = 0; index < 32; ++index) {
        const std::uint32_t first_value = first.nextUInt32();
        const std::uint32_t second_value = second.nextUInt32();
        const std::uint32_t different_value = different.nextUInt32();
        context.expectTrue(first_value == second_value,
                           "equal sampler seeds should reproduce the sequence");
        different_sequence_found |= first_value != different_value;
    }
    context.expectTrue(different_sequence_found,
                       "different sampler seeds should change the sequence");
}

void testPathSeedDerivation(nearlighter::test::Context& context) {
    const std::uint64_t base = derivePathSeed(7, 3, 5, 11);
    context.expectTrue(base == derivePathSeed(7, 3, 5, 11),
                       "path seed derivation should be deterministic");
    context.expectTrue(base != derivePathSeed(8, 3, 5, 11),
                       "render seed should affect a path seed");
    context.expectTrue(base != derivePathSeed(7, 4, 5, 11),
                       "pixel x should affect a path seed");
    context.expectTrue(base != derivePathSeed(7, 3, 6, 11),
                       "pixel y should affect a path seed");
    context.expectTrue(base != derivePathSeed(7, 3, 5, 12),
                       "sample index should affect a path seed");
}

void testSampleRanges(nearlighter::test::Context& context) {
    Sampler sampler(1234);
    for (int index = 0; index < 1000; ++index) {
        const float unit = sampler.next1D();
        context.expectTrue(unit >= 0.0f && unit < 1.0f,
                           "unit sample should remain in [0, 1)");

        const int integer = sampler.nextInt(-3, 4);
        context.expectTrue(integer >= -3 && integer <= 4,
                           "integer sample should remain in its inclusive range");
    }
    context.expectTrue(sampler.nextInt(5, 5) == 5,
                       "single-value integer range should return its bound");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testSequenceReproducibility(context);
    testPathSeedDerivation(context);
    testSampleRanges(context);
    return context.finish("sampler tests");
}
