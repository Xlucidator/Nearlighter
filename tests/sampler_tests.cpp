#include "test_support.h"

#include <nearlighter/sampling/sampler.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/shape/quad.h>
#include <cmath>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

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
}

void testDirectionDistributions(nearlighter::test::Context& context) {
    const SpherePDF sphere;
    const Vec3f normal = unit_vector(Vec3f(1, 2, 3));
    const CosineHemispherePDF cosine(normal);
    const MixturePDF mixture(sphere, cosine, 0.25f);
    const std::array<const PDF*, 3> distributions{&sphere, &cosine, &mixture};

    // Uniform solid-angle quadrature is independent of each distribution's generator.
    constexpr int height_steps = 128, azimuth_steps = 256;
    const double cell = 4.0 * pi / (height_steps * azimuth_steps);
    std::array<double, 3> integrals{};
    for (int i = 0; i < height_steps; ++i) {
        const float z = -1.0f + 2.0f * (i + 0.5f) / height_steps;
        const float radius = std::sqrt(1.0f - z * z);
        for (int j = 0; j < azimuth_steps; ++j) {
            const float phi = 2.0f * pi * (j + 0.5f) / azimuth_steps;
            const Vec3f direction(radius * std::cos(phi), radius * std::sin(phi), z);
            for (std::size_t k = 0; k < distributions.size(); ++k) {
                integrals[k] += distributions[k]->value(direction) * cell;
            }
        }
    }
    for (double integral : integrals) {
        context.expectNear(float(integral), 1.0f, 2e-4f, "continuous PDF integrates to one");
    }
    for (const PDF* distribution : distributions) {
        Sampler sampler(991);
        Vec3f mean;
        for (int i = 0; i < 32768; ++i) {
            const Vec3f direction = distribution->generate(sampler);
            context.expectNear(direction.length(), 1.0f, 1e-6f, "proposal produces unit directions");
            context.expectTrue(distribution->value(direction) > 0, "generated proposal has positive density");
            mean += direction;
        }
        const Vec3f expected = distribution == &sphere ? Vec3f()
            : normal * (distribution == &cosine ? 2.0f / 3.0f : 0.5f);
        context.expectVecNear(mean / 32768, expected, 0.015f,
                              "proposal moments match sphere, cosine, and weighted mixture laws");
        context.expectNear(distribution->value(Vec3f()), 0, 0, "zero direction has no density");
    }
    context.expectNear(mixture.value(normal), 0.25f / (4 * pi) + 0.75f / pi, 1e-7f,
                       "mixture evaluation uses the same unequal weights as generation");

    // Disjoint hemispheres expose mixture selection through generated directions.
    const CosineHemispherePDF opposite(-normal);
    const MixturePDF hemispheres(cosine, opposite, 0.25f);
    Sampler choices(573);
    int first_count = 0;
    for (int i = 0; i < 32768; ++i) {
        first_count += dot(hemispheres.generate(choices), normal) > 0.0f;
    }
    context.expectNear(float(first_count) / 32768, 0.25f, 0.01f,
                       "generated directions preserve the requested component probability");
}

void testDirectionScaleContract(nearlighter::test::Context& context) {
    const SpherePDF sphere;
    const Vec3f normal(0, 0, 1);
    const CosineHemispherePDF cosine(normal);
    const std::vector<std::shared_ptr<const Primitive>> targets{
        std::make_shared<Primitive>(
            std::make_shared<Quad>(Point3f(-1, -1, 3), Vec3f(2, 0, 0), Vec3f(0, 2, 0)),
            std::make_shared<Lambertian>(Color(1, 1, 1)))};
    const SurfacePDF surface(targets, Point3f());
    const MixturePDF mixture(cosine, surface);

    for (const PDF* distribution : std::array<const PDF*, 4>{&sphere, &cosine, &surface, &mixture}) {
        const float density = distribution->value(normal);
        for (float scale : {1e-9f, 0.1f, 1.0f, 100.0f, 1e9f}) {
            context.expectNear(distribution->value(scale * normal), density, 1e-6f,
                               "positive direction scaling must preserve solid-angle density");
        }
    }
}
}  // namespace

int main() {
    nearlighter::test::Context context;
    testSequenceReproducibility(context);
    testPathSeedDerivation(context);
    testSampleRanges(context);
    testDirectionDistributions(context);
    testDirectionScaleContract(context);
    return context.finish("sampler tests");
}
