#include "test_support.h"
#include <nearlighter/material/medium/isotropic.h>
#include <nearlighter/scene/object/instance.h>
#include <limits>


#include <nearlighter/base/interval.h>
#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/shape/sphere.h>

#include <algorithm>
#include <cmath>
#include <memory>


namespace {
/** Normalized Angular Response with a Deliberately Different Uniform Proposal */
class TestPhase final : public PhaseFunction {
public:
    float evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override {
        return (1.0f + 0.5f * dot(unit_vector(outgoing), unit_vector(incoming))) / (4.0f * pi);
    }
    float PDF([[maybe_unused]] const Vec3f& outgoing, const Vec3f& incoming) const override {
        return SpherePDF().value(incoming);
    }
    std::optional<PhaseSample> sample(const Vec3f& outgoing, Sampler& sampler) const override {
        const Vec3f incoming = SpherePDF().generate(sampler);
        return PhaseSample{evaluate(outgoing, incoming), incoming, PDF(outgoing, incoming)};
    }
};

void testMediumScaleAndPhase(nearlighter::test::Context& context) {
    const auto shape = std::make_shared<Sphere>(Point3f(), 1.0f);
    const auto material = std::make_shared<Homogeneous>(10.0f, Color(0.2f, 0.4f, 0.8f));
    const Transform scale = Transform::scale(Vec3f(2, 3, 4));
    const Primitive placed(shape, material, scale);
    const auto source = std::make_shared<Primitive>(shape, material);
    const Instance instance(source, scale);
    const Ray ray(Point3f(-4, 0, 0), Vec3f(2, 0, 0), 0.6f);
    Sampler expected(123), first(123), second(123);
    const float distance = -std::log(expected.next1D()) / 10.0f;
    HitRecord a, b;
    context.expectTrue(placed.hit(ray, Interval(0, infinity), a, first), "scaled shape scatters");
    context.expectTrue(instance.hit(ray, Interval(0, infinity), b, second), "instanced medium scatters");
    context.expectNear(a.t(), 1.0f + distance / 2.0f, 1e-5f,
                       "Primitive scale changes extent without changing input-space extinction");
    context.expectNear(b.t(), 1.0f + distance, 1e-5f,
                       "Instance scale places the entire medium including its distance units");
    const auto& event = b.medium();
    context.expectTrue(event.primitive == source.get() && event.material == material.get(),
                       "instance preserves volume identity and material binding");
    context.expectVecNear(event.outgoing, Vec3f(-1, 0, 0), 1e-6f, "volume outgoing uses caller space");
    const Ray oblique(Point3f(), Vec3f(1, 2, 3));
    Sampler angled(4);
    context.expectTrue(instance.hit(oblique, Interval(0, infinity), b, angled), "inside oblique event");
    context.expectVecNear(b.medium().outgoing, -unit_vector(oblique.direction()),
                          1e-6f, "nonuniform scaling recomputes unit outgoing from the parent ray");
    context.expectFalse(placed.hasPDF(), "medium extent is not a surface sampling target");

    const Material& root_material = *material;
    const auto scattering = root_material.scattering(b);
    context.expectTrue(std::holds_alternative<PhaseResponse>(scattering),
                       "the common material entry returns a volume response");
    const auto& response = std::get<PhaseResponse>(scattering);
    const Isotropic phase;
    Color mean;
    Vec3f direction_mean;
    double mean_z_squared = 0;
    Sampler samples(719);
    constexpr int count = 20000;
    for (int i = 0; i < count; ++i) {
        const auto sample = response.sample(Vec3f(0, 0, 2), samples);
        const float pdf = phase.PDF(Vec3f(0, 0, 2), sample->incoming);
        context.expectNear(sample->pdf, pdf, 1e-7f, "phase sample and PDF agree");
        context.expectNear(sample->incoming.length(), 1, 1e-6f, "phase returns a unit direction");
        mean += sample->value / sample->pdf;
        direction_mean += sample->incoming;
        mean_z_squared += sample->incoming.z() * sample->incoming.z();
    }
    // Uniform sphere moments are E[w] = 0 and E[z^2] = 1/3.
    context.expectVecNear(direction_mean / count, Vec3f(), 0.015f, "isotropic directional first moment");
    context.expectNear(float(mean_z_squared / count), 1.0f / 3.0f, 0.01f, "isotropic second moment");
    context.expectVecNear(mean / count, Color(0.2f, 0.4f, 0.8f), 0.001f,
                          "volume weight is albedo without a projected cosine");
    context.expectNear(phase.evaluate(Vec3f(), Vec3f(0, 0, 1)), 0, 0, "invalid phase direction is rejected");
    bool rejected = false;
    try { Homogeneous invalid(std::numeric_limits<float>::infinity(), Color(1, 1, 1)); }
    catch (const std::invalid_argument&) { rejected = true; }
    context.expectTrue(rejected, "nonfinite extinction is rejected before tracing");
}

void testPhaseResponseContract(nearlighter::test::Context& context) {
    const Isotropic phase;
    const Color albedo(0.2f, 0.4f, 0.8f);
    const PhaseResponse response(albedo, phase);
    const PhaseResponse absorbing(Color(), phase);
    const Vec3f outgoing(0, 1, 0), incoming(0, 0, 1);
    const SampleableFunction<Vec3f, Vec3f, Color, std::optional<ScatteringSample>>& function = response;
    context.expectVecNear(function.evaluate(outgoing, incoming), albedo / (4.0f * pi),
                          1e-7f, "complete response includes albedo");
    context.expectNear(function.PDF(outgoing, incoming), 1.0f / (4.0f * pi),
                       1e-7f, "albedo does not change the conditional direction PDF");
    context.expectNear(absorbing.PDF(outgoing, incoming), function.PDF(outgoing, incoming),
                       0, "zero scattering albedo preserves the proposal density");
    Sampler first(29), second(29), third(29);
    const auto sample = function.sample(outgoing, first);
    const auto phase_sample = phase.sample(outgoing, second);
    const auto zero_sample = absorbing.sample(outgoing, third);
    context.expectTrue(sample && phase_sample && zero_sample, "all proposals produce a direction");
    if (sample && phase_sample && zero_sample) {
        context.expectVecNear(sample->incoming, phase_sample->incoming, 0,
                              "response uses the phase proposal without extra random draws");
        context.expectVecNear(sample->value, albedo * phase_sample->value, 1e-7f,
                              "sample value includes albedo exactly once");
        context.expectNear(sample->pdf, phase_sample->pdf, 0,
                           "sample density excludes albedo");
        context.expectVecNear(zero_sample->value, Color(), 0, "pure absorption contributes zero");
    }
    bool rejected = false;
    try { PhaseResponse invalid(Color(1.1f, 0, 0), phase); }
    catch (const std::invalid_argument&) { rejected = true; }
    context.expectTrue(rejected, "invalid albedo cannot enter an evaluated response");

    const TestPhase unequal;
    const PhaseResponse weighted(albedo, unequal);
    const auto weighted_sample = weighted.sample(outgoing, first);
    context.expectVecNear(weighted_sample->value,
                          albedo * unequal.evaluate(outgoing, weighted_sample->incoming), 1e-7f,
                          "response value follows the phase even when its proposal differs");
    context.expectNear(weighted_sample->pdf, 1.0f / (4.0f * pi), 1e-7f,
                       "response density follows the proposal, not the phase value");
    context.expectVecNear(weighted.evaluate(outgoing, outgoing), albedo * (1.5f / (4.0f * pi)),
                          1e-7f, "phase response preserves nonuniform angular dependence");
}
}  // namespace

int main() {
    nearlighter::test::Context context;
    constexpr float density = 100.0f;
    const auto material = std::make_shared<Homogeneous>(density, Color(0.2f, 0.4f, 0.8f));
    const Primitive medium(std::make_shared<Sphere>(Point3f(), 1.0f), material);
    const Ray ray(Point3f(-2.0f, 0.0f, 0.0f),
                  Vec3f(1.0f, 0.0f, 0.0f));

    Sampler expected_sampler(123);
    const float random_value =
        std::max(expected_sampler.next1D(), 1e-7f);
    const float expected_t = 1.0f - std::log(random_value) / density;

    Sampler sampler(123);
    HitRecord record;
    const bool hit = medium.hit(ray, Interval(0.001f, infinity), record,
                                sampler);
    context.expectTrue(hit, "dense Homogeneous medium should scatter");
    if (hit) {
        context.expectNear(record.medium().t, expected_t, 1e-5f,
                           "Homogeneous medium fixed-seed free-flight distance");
        context.expectVecNear(record.medium().point, ray.at(expected_t), 1e-5f,
                              "Homogeneous medium interaction point");
        context.expectTrue(record.medium().material != nullptr,
                           "Homogeneous medium should bind its Material");
        context.expectTrue(!record.isSurface(),
                           "volume events use their own record type");
    }
    context.expectVecNear(medium.getBoundingBox().centroid(),
                          Point3f(0.0f, 0.0f, 0.0f), 1e-5f,
                          "Homogeneous medium should reuse boundary bounds");
    // Changing ray parameterization must preserve the physical event point.
    const Ray doubled(ray.origin(), 2.0f * ray.direction());
    Sampler doubled_sampler(123);
    HitRecord doubled_record;
    context.expectTrue(medium.hit(doubled, Interval(0.0f, infinity),
                                  doubled_record, doubled_sampler),
                       "non-unit ray should scatter");
    context.expectNear(doubled_record.medium().t, expected_t / 2.0f, 1e-5f,
                       "free flight converts physical distance to t");
    context.expectVecNear(doubled_record.medium().point, ray.at(expected_t), 1e-5f,
                          "direction magnitude preserves event position");

    Sampler inside_sampler(123);
    HitRecord inside_record;
    const Ray inside(Point3f(), Vec3f(1, 0, 0));
    context.expectTrue(medium.hit(inside, Interval(0.0f, infinity),
                                  inside_record, inside_sampler),
                       "inside origin should scatter after t zero");
    context.expectNear(inside_record.medium().t, expected_t - 1.0f, 1e-5f,
                       "inside origin clips the entry to zero");

    Sampler miss_sampler(123);
    HitRecord unchanged = record;
    context.expectFalse(medium.hit(ray, Interval(0.0f, 0.5f), unchanged,
                                   miss_sampler), "clipped segment should miss");
    context.expectNear(unchanged.medium().t, record.medium().t, 0.0f,
                       "miss preserves the caller record");
    testMediumScaleAndPhase(context);
    testPhaseResponseContract(context);
    return context.finish("Homogeneous medium tests");
}
