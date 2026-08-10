#include "test_support.h"

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>
#include <nearlighter/geometry/mesh.h>
#include <nearlighter/geometry/quad.h>
#include <nearlighter/geometry/sphere.h>
#include <nearlighter/geometry/triangle.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/transform/rotate.h>
#include <nearlighter/transform/scale.h>
#include <nearlighter/transform/translate.h>

#include <cmath>
#include <memory>

namespace {

constexpr float kTolerance = 1e-5f;

std::shared_ptr<Material> noMaterial() {
    return {};
}

void testSphere(nearlighter::test::Context& context) {
    Sampler sampler(0);
    Sphere sphere(Point3f(0.0f, 0.0f, -1.0f), 0.5f, noMaterial());

    HitRecord record;
    const bool hit = sphere.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record, sampler);
    context.expectTrue(hit, "sphere should be hit from outside");
    if (hit) {
        context.expectNear(record.t, 0.5f, kTolerance,
                           "outside sphere hit distance");
        context.expectVecNear(record.point, Point3f(0.0f, 0.0f, -0.5f),
                              kTolerance, "outside sphere hit point");
        context.expectVecNear(record.normal, Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "outside sphere normal");
        context.expectTrue(record.front_face,
                           "outside sphere hit should be front-facing");
    }

    HitRecord inside_record;
    const bool inside_hit = sphere.hit(
        Ray(Point3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 0.0f, 0.0f)),
        Interval(0.001f, infinity), inside_record, sampler);
    context.expectTrue(inside_hit, "sphere should be hit from inside");
    if (inside_hit) {
        context.expectNear(inside_record.t, 0.5f, kTolerance,
                           "inside sphere hit distance");
        context.expectVecNear(inside_record.normal, Vec3f(-1.0f, 0.0f, 0.0f),
                              kTolerance, "inside sphere normal orientation");
        context.expectFalse(inside_record.front_face,
                            "inside sphere hit should be back-facing");
    }

    HitRecord miss_record;
    context.expectFalse(
        sphere.hit(
            Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f)),
            Interval(0.001f, infinity), miss_record, sampler),
        "sphere miss ray should not hit");
}

void testQuad(nearlighter::test::Context& context) {
    Sampler sampler(0);
    Quad quad(Point3f(-1.0f, -1.0f, -1.0f),
              Vec3f(2.0f, 0.0f, 0.0f),
              Vec3f(0.0f, 2.0f, 0.0f), noMaterial());

    HitRecord record;
    const bool hit = quad.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record, sampler);
    context.expectTrue(hit, "quad center ray should hit");
    if (hit) {
        context.expectNear(record.t, 1.0f, kTolerance,
                           "quad hit distance");
        context.expectVecNear(record.point, Point3f(0.0f, 0.0f, -1.0f),
                              kTolerance, "quad hit point");
        context.expectVecNear(record.normal, Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "quad normal");
        context.expectNear(record.u, 0.5f, kTolerance, "quad u coordinate");
        context.expectNear(record.v, 0.5f, kTolerance, "quad v coordinate");
    }

    HitRecord miss_record;
    context.expectFalse(
        quad.hit(
            Ray(Point3f(2.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
            Interval(0.001f, infinity), miss_record, sampler),
        "ray outside quad boundary should miss");
}

void testTriangle(nearlighter::test::Context& context) {
    Sampler sampler(0);
    Triangle triangle(Point3f(0.0f, 0.0f, -1.0f),
                      Point3f(2.0f, 0.0f, -1.0f),
                      Point3f(0.0f, 2.0f, -1.0f), noMaterial());

    HitRecord record;
    const bool hit = triangle.hit(
        Ray(Point3f(0.5f, 0.5f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record, sampler);
    context.expectTrue(hit, "triangle interior ray should hit");
    if (hit) {
        context.expectNear(record.t, 1.0f, kTolerance,
                           "triangle hit distance");
        context.expectNear(record.u, 0.25f, kTolerance,
                           "triangle first barycentric coordinate");
        context.expectNear(record.v, 0.25f, kTolerance,
                           "triangle second barycentric coordinate");
        context.expectVecNear(record.normal, Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "triangle front normal");
        context.expectTrue(record.front_face,
                           "triangle front ray should be front-facing");
    }

    HitRecord back_record;
    const bool back_hit = triangle.hit(
        Ray(Point3f(0.5f, 0.5f, -2.0f), Vec3f(0.0f, 0.0f, 1.0f)),
        Interval(0.001f, infinity), back_record, sampler);
    context.expectTrue(back_hit, "triangle should intersect from both sides");
    if (back_hit) {
        context.expectVecNear(back_record.normal, Vec3f(0.0f, 0.0f, -1.0f),
                              kTolerance, "triangle back normal orientation");
        context.expectFalse(back_record.front_face,
                            "triangle back ray should be back-facing");
    }

    HitRecord miss_record;
    context.expectFalse(
        triangle.hit(
            Ray(Point3f(1.5f, 1.5f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
            Interval(0.001f, infinity), miss_record, sampler),
        "ray outside triangle boundary should miss");

    context.expectNear(
        triangle.getPDFValue(Point3f(0.5f, 0.5f, 0.0f),
                             Vec3f(0.0f, 0.0f, -1.0f)),
        0.5f, kTolerance, "triangle area-sampling PDF");
    const Vec3f sampled_direction =
        triangle.random(Point3f(0.0f, 0.0f, 0.0f), sampler);
    context.expectTrue(
        sampled_direction.x() >= 0.0f && sampled_direction.y() >= 0.0f &&
            sampled_direction.x() + sampled_direction.y() <= 2.0f &&
            std::fabs(sampled_direction.z() + 1.0f) <= kTolerance,
        "triangle random direction should reach its surface");
}

void testMesh(nearlighter::test::Context& context) {
    MeshData data;
    data.positions = {
        Point3f(-1.0f, -1.0f, -1.0f),
        Point3f(1.0f, -1.0f, -1.0f),
        Point3f(1.0f, 1.0f, -1.0f),
        Point3f(-1.0f, 1.0f, -1.0f),
    };
    data.texture_coordinates = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
    };
    data.triangles = {{0, 1, 2}, {0, 2, 3}};
    Mesh mesh(std::move(data), noMaterial(), MeshBuildOptions{true});

    Sampler sampler(0);
    HitRecord record;
    const bool hit = mesh.hit(
        Ray(Point3f(0.25f, -0.25f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record, sampler);
    context.expectTrue(hit, "mesh BVH should hit an indexed triangle");
    if (hit) {
        context.expectVecNear(record.point, Point3f(0.25f, -0.25f, -1.0f),
                              kTolerance, "mesh hit point");
        context.expectVecNear(record.normal, Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "generated mesh normal");
        context.expectNear(record.u, 0.625f, kTolerance,
                           "interpolated mesh u coordinate");
        context.expectNear(record.v, 0.375f, kTolerance,
                           "interpolated mesh v coordinate");
    }
}

void testAabb(nearlighter::test::Context& context) {
    const AABB box(Point3f(-1.0f, -1.0f, -1.0f),
                   Point3f(1.0f, 1.0f, 1.0f));

    context.expectTrue(
        box.hit(Ray(Point3f(0.0f, 0.0f, -3.0f), Vec3f(0.0f, 0.0f, 1.0f)),
                Interval(0.0f, infinity)),
        "ray through AABB should hit");
    context.expectFalse(
        box.hit(Ray(Point3f(2.0f, 0.0f, -3.0f), Vec3f(0.0f, 0.0f, 1.0f)),
                Interval(0.0f, infinity)),
        "parallel ray outside AABB slab should miss");
}

void testTransforms(nearlighter::test::Context& context) {
    Sampler sampler(0);
    // Transform wrappers intersect in object space, then restore the hit point
    // and normal to world space. Check both values to cover that round trip.
    auto sphere = std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -1.0f), 0.5f, noMaterial());
    Translate translated(sphere, Vec3f(2.0f, 0.0f, 0.0f));

    HitRecord translated_record;
    const bool translated_hit = translated.hit(
        Ray(Point3f(2.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), translated_record, sampler);
    context.expectTrue(translated_hit, "translated sphere should be hit");
    if (translated_hit) {
        context.expectVecNear(translated_record.point,
                              Point3f(2.0f, 0.0f, -0.5f),
                              kTolerance, "translated sphere hit point");
        context.expectVecNear(translated_record.normal,
                              Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "translated sphere normal");
    }

    auto offset_sphere = std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -2.0f), 0.5f, noMaterial());
    Rotate rotated(offset_sphere, Vec3f(0.0f, 1.0f, 0.0f), pi * 0.5f);

    HitRecord rotated_record;
    const bool rotated_hit = rotated.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(-1.0f, 0.0f, 0.0f)),
        Interval(0.001f, infinity), rotated_record, sampler);
    context.expectTrue(rotated_hit, "rotated sphere should be hit");
    if (rotated_hit) {
        context.expectNear(rotated_record.t, 1.5f, kTolerance,
                           "rotated sphere hit distance");
        context.expectVecNear(rotated_record.point,
                              Point3f(-1.5f, 0.0f, 0.0f),
                              kTolerance, "rotated sphere hit point");
        context.expectVecNear(rotated_record.normal,
                              Vec3f(1.0f, 0.0f, 0.0f),
                              kTolerance, "rotated sphere normal");
    }

    auto unit_sphere = std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -1.0f), 0.5f, noMaterial());
    Scale scaled(unit_sphere, 2.0f);

    HitRecord scaled_record;
    const bool scaled_hit = scaled.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), scaled_record, sampler);
    context.expectTrue(scaled_hit, "scaled sphere should be hit");
    if (scaled_hit) {
        context.expectNear(scaled_record.t, 1.0f, kTolerance,
                           "scaled sphere hit distance");
        context.expectVecNear(scaled_record.point,
                              Point3f(0.0f, 0.0f, -1.0f),
                              kTolerance, "scaled sphere hit point");
        context.expectVecNear(scaled_record.normal,
                              Vec3f(0.0f, 0.0f, 1.0f),
                              kTolerance, "scaled sphere normal");
    }
    context.expectVecNear(scaled.getBoundingBox().centroid(),
                          Point3f(0.0f, 0.0f, -2.0f), kTolerance,
                          "scaled sphere bounding-box center");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testSphere(context);
    testQuad(context);
    testTriangle(context);
    testMesh(context);
    testAabb(context);
    testTransforms(context);
    return context.finish("geometry tests");
}
