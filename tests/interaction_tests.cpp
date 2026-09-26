#include "test_support.h"

#include <nearlighter/material/lambertian.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/shape/sphere.h>

#include <memory>
#include <stdexcept>

namespace {

constexpr float kTolerance = 1e-5f;

void testShadingFrame(nearlighter::test::Context& context) {
    const ShadingFrame frame(unit_vector(Vec3f(1.0f, -2.0f, 3.0f)));

    context.expectNear(dot(frame.tangent(), frame.bitangent()), 0.0f,
                       kTolerance, "frame tangent axes should be orthogonal");
    context.expectVecNear(cross(frame.tangent(), frame.bitangent()),
                          frame.normal(), kTolerance,
                          "frame axes should remain right-handed");

    const Vec3f world(0.25f, -0.5f, 0.75f);
    context.expectVecNear(frame.toWorld(frame.toLocal(world)), world,
                          kTolerance,
                          "frame conversion should round-trip directions");
    context.expectVecNear(frame.toLocal(frame.normal()),
                          Vec3f(0.0f, 0.0f, 1.0f), kTolerance,
                          "frame normal should map to local positive z");
}

void testSurfaceInteraction(nearlighter::test::Context& context) {
    const auto material =
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f));
    const Primitive primitive(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, 0.0f), 1.0f),
        material);
    const Ray incident(Point3f(0.0f, 0.0f, 3.0f),
                       Vec3f(0.0f, 0.0f, -1.0f), 0.25f);
    Sampler sampler(0);
    HitRecord record;
    const bool hit = primitive.hit(
        incident, Interval(0.0f, infinity), record, sampler);
    context.expectTrue(hit, "surface fixture should be intersected");
    if (!hit) return;

    const SurfaceInteraction interaction(incident, record);
    context.expectVecNear(interaction.point(), Point3f(0.0f, 0.0f, 1.0f),
                          kTolerance, "interaction should preserve hit point");
    context.expectVecNear(interaction.outgoing(), Vec3f(0.0f, 0.0f, 1.0f),
                          kTolerance,
                          "interaction outgoing should point to previous vertex");
    context.expectTrue(&interaction.primitive() == &primitive,
                       "interaction should retain its source Primitive");

    const Ray outward = interaction.spawnRay(Vec3f(0.0f, 0.0f, 1.0f),
                                              incident.time());
    context.expectTrue(outward.origin().z() > interaction.point().z(),
                       "outward ray should start above the surface");
    HitRecord outward_record;
    context.expectFalse(
        primitive.hit(outward, Interval(0.0f, infinity), outward_record,
                      sampler),
        "outward spawned ray should not self-intersect the sphere");

    const Ray inward = interaction.spawnRay(Vec3f(0.0f, 0.0f, -1.0f),
                                             incident.time());
    HitRecord inward_record;
    context.expectTrue(
        primitive.hit(inward, Interval(0.0f, infinity), inward_record,
                      sampler),
        "inward spawned ray should reach the opposite surface");
    context.expectTrue(inward_record.t > 1.9f,
                       "inward ray should not immediately hit its origin");

    HitRecord medium_record = record;
    medium_record.kind = InteractionKind::Medium;
    medium_record.primitive = nullptr;
    bool medium_rejected = false;
    try {
        const SurfaceInteraction invalid(incident, medium_record);
        (void)invalid;
    } catch (const std::invalid_argument&) {
        medium_rejected = true;
    }
    context.expectTrue(medium_rejected,
                       "SurfaceInteraction should reject medium records");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testShadingFrame(context);
    testSurfaceInteraction(context);
    return context.finish("interaction tests");
}
