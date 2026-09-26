#include "test_support.h"

#include <nearlighter/material/material.h>
#include <nearlighter/material/dielectric.h>
#include <nearlighter/material/emissive.h>
#include <nearlighter/material/isotropic.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/material/metal.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/texture/texture.h>

#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

constexpr float kTolerance = 1e-5f;

static_assert(std::is_abstract_v<BxDF>);
static_assert(!std::is_copy_constructible_v<BSDF>);
static_assert(!std::is_copy_assignable_v<BSDF>);
static_assert(std::is_nothrow_move_constructible_v<BSDF>);
static_assert(std::is_nothrow_move_assignable_v<BSDF>);

/** Test-Only Composition through the Ordinary Material Entry */
class MixedMaterial final : public Material {
public:
    std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction,
        [[maybe_unused]] TransportMode mode) const override {
        BSDF bsdf(interaction.frame());
        bsdf.add(std::make_unique<LambertianBxDF>(Color(0.2f, 0.2f, 0.2f)));
        bsdf.add(std::make_unique<SpecularReflectionBxDF>(Color(0.3f, 0.3f, 0.3f)));
        return bsdf;
    }

    bool supportsPathIntegrator() const override { return true; }
};

/** Position-Dependent Colors for Opposite Unit-Sphere Hits */
class PositionTexture final : public Texture {
public:
    Color value([[maybe_unused]] float u, [[maybe_unused]] float v,
                const Point3f& point) const override {
        return 0.25f * (point + Vec3f(1.0f, 1.0f, 1.0f));
    }
};

/** Destruction Probe for Exclusive Polymorphic Ownership */
class TrackedDiffuseBxDF final : public LambertianBxDF {
public:
    explicit TrackedDiffuseBxDF(int& destructions)
        : LambertianBxDF(Color(0.2f, 0.2f, 0.2f)), destructions_(destructions) {}
    ~TrackedDiffuseBxDF() override { ++destructions_; }

private:
    int& destructions_;
};

BSDF makeDiffuseBSDF(const Color& albedo) {
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    bsdf.add(std::make_unique<LambertianBxDF>(albedo));
    return bsdf;
}

void testLambertianEvaluation(nearlighter::test::Context& context) {
    const Color albedo(0.2f, 0.4f, 0.8f);
    const BSDF bsdf = makeDiffuseBSDF(albedo);
    const Vec3f outgoing(0.0f, 0.0f, 1.0f);
    const Vec3f incoming = unit_vector(Vec3f(0.0f, 1.0f, 1.0f));

    context.expectVecNear(
        bsdf.evaluate(outgoing, incoming, TransportMode::Radiance),
        albedo / pi, kTolerance,
        "Lambertian value should equal albedo divided by pi");
    context.expectNear(
        bsdf.PDF(outgoing, incoming, TransportMode::Radiance),
        incoming.z() / pi, kTolerance,
        "Lambertian PDF should be cosine weighted");
    context.expectVecNear(
        bsdf.evaluate(outgoing, -incoming, TransportMode::Radiance), Color(),
        kTolerance, "Lambertian component should reject the opposite hemisphere");
}

void testLambertianPDFNormalization(nearlighter::test::Context& context) {
    const BSDF bsdf = makeDiffuseBSDF(Color(0.5f, 0.5f, 0.5f));
    const Vec3f outgoing(0.0f, 0.0f, 1.0f);
    constexpr int kCosineSteps = 128;
    constexpr int kAzimuthSteps = 128;
    const float solid_angle = 2.0f * pi /
                              static_cast<float>(kCosineSteps * kAzimuthSteps);
    float integral = 0.0f;

    for (int z_index = 0; z_index < kCosineSteps; ++z_index) {
        const float cosine =
            (static_cast<float>(z_index) + 0.5f) / kCosineSteps;
        const float sine = std::sqrt(1.0f - cosine * cosine);
        for (int phi_index = 0; phi_index < kAzimuthSteps; ++phi_index) {
            const float phi = 2.0f * pi *
                              (static_cast<float>(phi_index) + 0.5f) /
                              kAzimuthSteps;
            const Vec3f incoming(sine * std::cos(phi),
                                 sine * std::sin(phi), cosine);
            integral += bsdf.PDF(outgoing, incoming,
                                 TransportMode::Radiance) * solid_angle;
        }
    }

    context.expectNear(integral, 1.0f, 2e-4f,
                       "Lambertian PDF should integrate to one hemisphere");
}

void testIdealReflection(nearlighter::test::Context& context) {
    const Color reflectance(0.25f, 0.5f, 0.75f);
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    bsdf.add(std::make_unique<SpecularReflectionBxDF>(reflectance));
    const Vec3f outgoing = unit_vector(Vec3f(0.2f, -0.3f, 1.0f));
    const auto sample = bsdf.sample(outgoing, 0.0f, Vec2f(0.3f, 0.7f),
                                    TransportMode::Radiance);

    context.expectTrue(sample.has_value(),
                       "ideal mirror should produce a delta sample");
    if (!sample) return;
    context.expectVecNear(
        sample->incoming,
        Vec3f(-outgoing.x(), -outgoing.y(), outgoing.z()), kTolerance,
        "ideal mirror should mirror the local tangent components");
    context.expectNear(sample->pdf, 1.0f, kTolerance,
                       "single mirror component should have unit event PDF");
    context.expectTrue(
        hasAnyFlag(sample->flags, BxDFFlags::Reflection) &&
            hasAnyFlag(sample->flags, BxDFFlags::Specular),
        "mirror sample should be marked as specular reflection");
    context.expectVecNear(
        sample->value * std::fabs(sample->incoming.z()) / sample->pdf,
        reflectance, kTolerance,
        "mirror throughput update should recover its reflectance");
    context.expectNear(
        bsdf.PDF(outgoing, sample->incoming, TransportMode::Radiance), 0.0f,
        0.0f, "delta mirror should have zero solid-angle density");
}

void testIdealDielectric(nearlighter::test::Context& context) {
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    bsdf.add(std::make_unique<SpecularDielectricBxDF>(1.0f, 1.5f));
    const Vec3f outgoing(0.0f, 0.0f, 1.0f);
    const float reflectance = 0.04f;

    const auto reflected = bsdf.sample(
        outgoing, 0.0f, Vec2f(0.0f, 0.5f), TransportMode::Radiance);
    context.expectTrue(reflected.has_value(),
                       "dielectric should produce a reflection sample");
    if (reflected) {
        context.expectVecNear(reflected->incoming, outgoing, kTolerance,
                              "normal-incidence reflection should reverse travel");
        context.expectNear(reflected->pdf, reflectance, kTolerance,
                           "reflection event PDF should equal Fresnel weight");
    }

    const auto transmitted = bsdf.sample(
        outgoing, 0.0f, Vec2f(0.5f, 0.5f), TransportMode::Radiance);
    context.expectTrue(transmitted.has_value(),
                       "dielectric should produce a transmission sample");
    if (transmitted) {
        context.expectVecNear(transmitted->incoming, Vec3f(0.0f, 0.0f, -1.0f),
                              kTolerance,
                              "normal-incidence transmission should cross the surface");
        context.expectNear(transmitted->pdf, 1.0f - reflectance, kTolerance,
                           "transmission event PDF should equal Fresnel complement");
        context.expectNear(transmitted->eta, 1.5f, kTolerance,
                           "transmission should report the relative index");
        const Color throughput = transmitted->value *
                                 std::fabs(transmitted->incoming.z()) /
                                 transmitted->pdf;
        context.expectVecNear(
            throughput, Color(1.0f / 2.25f, 1.0f / 2.25f, 1.0f / 2.25f),
            kTolerance,
            "radiance transmission should include the squared eta factor");
    }
}

void testTotalInternalReflection(nearlighter::test::Context& context) {
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    bsdf.add(std::make_unique<SpecularDielectricBxDF>(1.5f, 1.0f));
    const Vec3f outgoing = unit_vector(Vec3f(0.9f, 0.0f, 0.4f));
    const auto sample = bsdf.sample(outgoing, 0.0f, Vec2f(0.99f, 0.5f),
                                    TransportMode::Radiance);

    context.expectTrue(sample.has_value(),
                       "total internal reflection should remain sampleable");
    if (!sample) return;
    context.expectNear(sample->pdf, 1.0f, kTolerance,
                       "total internal reflection should have unit event PDF");
    context.expectTrue(hasAnyFlag(sample->flags, BxDFFlags::Reflection),
                       "total internal reflection should select reflection");
}

void testFixedComponentCapacity(nearlighter::test::Context& context) {
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    for (std::size_t index = 0; index < BSDF::kMaximumComponents; ++index) {
        bsdf.add(std::make_unique<LambertianBxDF>(Color(0.1f, 0.1f, 0.1f)));
    }

    bool rejected = false;
    try {
        bsdf.add(std::make_unique<LambertianBxDF>(Color(0.1f, 0.1f, 0.1f)));
    } catch (const std::length_error&) {
        rejected = true;
    }
    context.expectTrue(rejected,
                       "BSDF should reject components beyond its fixed capacity");
    context.expectTrue(bsdf.componentCount() == BSDF::kMaximumComponents,
                       "overflow must not change the number of live components");
    rejected = false;
    try {
        bsdf.add(nullptr);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    context.expectTrue(rejected, "null insertion must be rejected without a counted empty slot");
    context.expectVecNear(
        bsdf.evaluate(Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 0.0f, 1.0f),
                      TransportMode::Radiance),
        Color(0.4f, 0.4f, 0.4f) / pi, kTolerance,
        "failed insertions must preserve existing component responses");
}

void testContinuousMixture(nearlighter::test::Context& context) {
    BSDF bsdf(ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f)));
    const Color first(0.1f, 0.2f, 0.3f);
    const Color second(0.3f, 0.2f, 0.1f);
    bsdf.add(std::make_unique<LambertianBxDF>(first));
    bsdf.add(std::make_unique<LambertianBxDF>(second));
    for (float selection : {0.25f, 0.75f}) {
        const auto sample = bsdf.sample(Vec3f(0.0f, 0.0f, 1.0f), selection,
                                        Vec2f(0.2f, 0.4f), TransportMode::Radiance);
        context.expectTrue(sample.has_value(), "either diffuse component should sample");
        if (!sample) continue;
        context.expectVecNear(sample->value, (first + second) / pi, kTolerance,
                              "continuous samples must sum all component values");
        context.expectNear(sample->pdf, sample->incoming.z() / pi, kTolerance,
                           "identical proposals must average, not sum, their PDFs");
    }
}

void testMixedMeasures(nearlighter::test::Context& context) {
    const Vec3f outgoing(0.0f, 0.0f, 1.0f);
    const Color albedo(0.2f, 0.3f, 0.4f);
    const Color mirror(0.4f, 0.3f, 0.2f);
    BSDF bsdf{ShadingFrame(outgoing)};
    bsdf.add(std::make_unique<LambertianBxDF>(albedo));
    bsdf.add(std::make_unique<SpecularReflectionBxDF>(mirror));
    const auto diffuse = bsdf.sample(outgoing, 0.25f, Vec2f(0.2f, 0.4f),
                                     TransportMode::Radiance);
    context.expectTrue(diffuse.has_value(), "mixed BSDF should sample its diffuse component");
    if (diffuse) {
        context.expectVecNear(diffuse->value, albedo / pi, kTolerance,
                              "delta components have no continuous value");
        context.expectNear(diffuse->pdf, 0.5f * diffuse->incoming.z() / pi, kTolerance,
                           "continuous PDF must include component selection");
    }
    const auto specular = bsdf.sample(outgoing, 0.75f, Vec2f(0.2f, 0.4f),
                                      TransportMode::Radiance);
    context.expectTrue(specular.has_value(), "mixed BSDF should sample its delta component");
    if (specular) {
        context.expectVecNear(specular->value, mirror, kTolerance,
                              "delta samples must retain only the selected event coefficient");
        context.expectNear(specular->pdf, 0.5f, kTolerance,
                           "delta probability must include component selection");
    }

    // Coincident delta directions still represent separately selected components.
    BSDF delta_only{ShadingFrame(outgoing)};
    delta_only.add(std::make_unique<SpecularReflectionBxDF>(albedo));
    delta_only.add(std::make_unique<SpecularReflectionBxDF>(mirror));
    Color expected_contribution;
    for (float selection : {0.25f, 0.75f}) {
        const auto sample = delta_only.sample(outgoing, selection, Vec2f(0.2f, 0.4f),
                                              TransportMode::Radiance);
        context.expectTrue(sample.has_value(), "either delta component should sample");
        if (!sample) continue;
        context.expectNear(sample->pdf, 0.5f, kTolerance,
                           "each mirror event should carry its selection probability");
        expected_contribution += 0.5f * sample->value / sample->pdf;
    }
    context.expectVecNear(expected_contribution, albedo + mirror, kTolerance,
                          "component selection must preserve expected delta throughput");
}

void testRotatedFrameAndImportance(nearlighter::test::Context& context) {
    const Vec3f normal = unit_vector(Vec3f(1.0f, 2.0f, 3.0f));
    const Color albedo(0.2f, 0.4f, 0.6f);
    BSDF diffuse{ShadingFrame(normal)};
    diffuse.add(std::make_unique<LambertianBxDF>(albedo));
    const auto sample = diffuse.sample(normal, 0.0f, Vec2f(0.2f, 0.4f),
                                        TransportMode::Radiance);
    context.expectTrue(sample.has_value(), "rotated frame should remain sampleable");
    if (sample) {
        context.expectNear(sample->pdf, dot(sample->incoming, normal) / pi, kTolerance,
                           "world-space PDF must use the shading normal, not world z");
        context.expectNear(diffuse.PDF(normal, sample->incoming, TransportMode::Radiance),
                           sample->pdf, kTolerance, "rotated sample and queried PDF must agree");
        context.expectVecNear(diffuse.evaluate(normal, sample->incoming, TransportMode::Radiance),
                              sample->value, kTolerance, "rotated sample and evaluation must agree");
    }
    BSDF glass{ShadingFrame(normal)};
    glass.add(std::make_unique<SpecularDielectricBxDF>(1.0f, 1.5f));
    const auto transmitted = glass.sample(normal, 0.0f, Vec2f(0.5f, 0.5f),
                                          TransportMode::Importance);
    context.expectTrue(transmitted.has_value(), "importance transport should transmit");
    if (!transmitted) return;
    context.expectVecNear(transmitted->incoming, -normal, kTolerance,
                          "transmitted continuation must cross the rotated surface");
    context.expectVecNear(transmitted->value * std::fabs(dot(transmitted->incoming, normal)) /
                              transmitted->pdf,
                          Color(1.0f, 1.0f, 1.0f), kTolerance,
                          "importance transmission must omit the radiance eta-squared factor");
}

void testMoveOwnership(nearlighter::test::Context& context) {
    int destructions = 0;
    const Vec3f normal = unit_vector(Vec3f(1.0f, 2.0f, 3.0f));
    {
        BSDF source{ShadingFrame(normal)};
        source.add(std::make_unique<TrackedDiffuseBxDF>(destructions));
        source.add(std::make_unique<TrackedDiffuseBxDF>(destructions));
        BSDF moved(std::move(source));
        context.expectTrue(source.empty() && moved.componentCount() == 2,
                           "move construction must clear the source count");
        context.expectVecNear(source.evaluate(normal, normal, TransportMode::Radiance),
                              Color(), 0.0f, "moved-from evaluation must be empty");
        context.expectNear(source.PDF(normal, normal, TransportMode::Radiance),
                           0.0f, 0.0f, "moved-from PDF must be zero");
        context.expectFalse(source.sample(normal, 0.0f, Vec2f(0.2f, 0.4f),
                                          TransportMode::Radiance).has_value(),
                            "moved-from sampling must not dereference transferred pointers");

        BSDF assigned{ShadingFrame(Vec3f(0.0f, 0.0f, 1.0f))};
        assigned.add(std::make_unique<TrackedDiffuseBxDF>(destructions));
        assigned = std::move(moved);
        context.expectTrue(moved.empty() && assigned.componentCount() == 2,
                           "move assignment must transfer the entire aggregate");
        context.expectTrue(destructions == 1,
                           "move assignment must release the previous destination component");

        BSDF& self = assigned;
        assigned = std::move(self);
        context.expectVecNear(assigned.evaluate(normal, normal, TransportMode::Radiance),
                              Color(0.4f, 0.4f, 0.4f) / pi, kTolerance,
                              "self-move must preserve the response");
        const auto sample = assigned.sample(normal, 0.75f, Vec2f(0.2f, 0.4f),
                                             TransportMode::Radiance);
        context.expectTrue(sample.has_value(), "moved aggregate should remain sampleable");
        if (sample) {
            context.expectNear(sample->pdf, dot(normal, sample->incoming) / pi,
                               kTolerance, "move assignment must transfer the rotated frame");
        }
    }
    context.expectTrue(destructions == 3,
                       "each polymorphic component must be destroyed exactly once");
}

void testMaterialContracts(nearlighter::test::Context& context) {
    const auto mixed = std::make_shared<MixedMaterial>();
    const Primitive primitive(std::make_shared<Sphere>(Point3f(), 1.0f), mixed);
    const Ray ray(Point3f(0.0f, 0.0f, 3.0f), Vec3f(0.0f, 0.0f, -1.0f));
    HitRecord record;
    Sampler sampler(0);
    const bool hit = primitive.hit(ray, Interval(0.0f, infinity), record, sampler);
    context.expectTrue(hit, "material contract fixture should intersect");
    if (!hit) return;
    const SurfaceInteraction interaction(ray, record);

    const auto bsdf = interaction.material().computeBSDF(interaction, TransportMode::Radiance);
    context.expectTrue(bsdf && bsdf->componentCount() == 2,
                       "a user material should compose components without central dispatch edits");
    if (bsdf) {
        const auto sample = bsdf->sample(interaction.outgoing(), 0.75f, Vec2f(0.2f, 0.4f),
                                         TransportMode::Radiance);
        context.expectTrue(sample && hasAnyFlag(sample->flags, BxDFFlags::Specular),
                           "the material entry should preserve its mirror component");
    }

    Material unsupported;
    Isotropic volume(Color(0.5f, 0.5f, 0.5f));
    Metal fuzzy(Color(0.5f, 0.5f, 0.5f), 0.3f);
    for (const Material* material : std::array<const Material*, 3>{&unsupported, &volume, &fuzzy}) {
        bool rejected = false;
        try {
            material->computeBSDF(interaction, TransportMode::Radiance);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        context.expectTrue(rejected,
                           "unsupported scattering must throw instead of silently absorbing");
    }
    Emissive emitter(Color(3.0f, 3.0f, 3.0f));
    context.expectFalse(emitter.computeBSDF(interaction, TransportMode::Radiance).has_value(),
                        "a supported pure emitter must explicitly return no scattering");
    context.expectVecNear(emitter.evaluateEmission(interaction, interaction.outgoing()),
                          Color(3.0f, 3.0f, 3.0f), kTolerance,
                          "ending scattering must not suppress emission");
    context.expectVecNear(emitter.evaluateEmission(interaction, -interaction.outgoing()),
                          Color(), 0.0f, "emission must remain one-sided");
}

void testIndependentHitParameters(nearlighter::test::Context& context) {
    std::array<std::optional<BSDF>, 2> responses;
    const std::array<Vec3f, 2> normals{Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 0.0f, -1.0f)};
    {
        const auto material = std::make_shared<Lambertian>(std::make_shared<PositionTexture>());
        const Primitive primitive(std::make_shared<Sphere>(Point3f(), 1.0f), material);
        Sampler sampler(0);
        for (std::size_t index = 0; index < normals.size(); ++index) {
            const Ray ray(3.0f * normals[index], -normals[index]);
            HitRecord record;
            const bool hit = primitive.hit(ray, Interval(0.0f, infinity), record, sampler);
            context.expectTrue(hit, "textured material fixture should intersect");
            if (!hit) return;
            const SurfaceInteraction interaction(ray, record);
            responses[index] = material->computeBSDF(interaction, TransportMode::Radiance);
        }
    }
    // Both interactions, their primitive, material, and texture are now gone.
    for (std::size_t index = 0; index < normals.size(); ++index) {
        context.expectTrue(responses[index].has_value(), "textured hit should own a BSDF");
        if (!responses[index]) continue;
        const Color expected = 0.25f * (normals[index] + Vec3f(1.0f, 1.0f, 1.0f));
        context.expectVecNear(
            responses[index]->evaluate(normals[index], normals[index], TransportMode::Radiance),
            expected / pi, kTolerance,
            "each BSDF must retain its own evaluated color beyond the interaction lifetime");
    }
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testLambertianEvaluation(context);
    testLambertianPDFNormalization(context);
    testIdealReflection(context);
    testIdealDielectric(context);
    testTotalInternalReflection(context);
    testFixedComponentCapacity(context);
    testContinuousMixture(context);
    testMixedMeasures(context);
    testRotatedFrameAndImportance(context);
    testMoveOwnership(context);
    testMaterialContracts(context);
    testIndependentHitParameters(context);
    return context.finish("BSDF tests");
}
