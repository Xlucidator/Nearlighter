#include "test_support.h"
#include <nearlighter/accel/bvh.h>
#include <nearlighter/scene/object/instance.h>
#include <nearlighter/scene/object/linear_aggregate.h>
#include <nearlighter/render/hit/surface_intersection.h>
#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/texture/texture.h>


#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/render/hit/hit_record.h>
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

void testSurfaceRecord(nearlighter::test::Context& context) {
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

    const auto& interaction = record.surface();
    context.expectVecNear(record.point(), Point3f(0.0f, 0.0f, 1.0f),
                          kTolerance, "interaction should preserve hit point");
    context.expectVecNear(record.outgoing(), Vec3f(0.0f, 0.0f, 1.0f),
                          kTolerance,
                          "interaction outgoing should point to previous vertex");
    context.expectTrue(&record.primitive() == &primitive &&
                           &record.material() == material.get(),
                       "interaction should retain its source Primitive");
    context.expectNear(record.t(), 2.0f, 0.0f,
                       "hit parameter remains independent of scene time");

    const Ray outward = record.spawnRay(Vec3f(0.0f, 0.0f, 1.0f), incident.time());
    context.expectTrue(outward.origin().z() > interaction.point.z(),
                       "outward ray should start above the surface");
    HitRecord outward_record;
    context.expectFalse(
        primitive.hit(outward, Interval(0.0f, infinity), outward_record,
                      sampler),
        "outward spawned ray should not self-intersect the sphere");

    const Ray inward = record.spawnRay(Vec3f(0.0f, 0.0f, -1.0f), incident.time());
    HitRecord inward_record;
    context.expectTrue(
        primitive.hit(inward, Interval(0.0f, infinity), inward_record,
                      sampler),
        "inward spawned ray should reach the opposite surface");
    context.expectTrue(inward_record.surface().t > 1.9f,
                       "inward ray should not immediately hit its origin");

    MediumRecord volume;
    volume.point = Point3f(1, 2, 3);
    volume.outgoing = Vec3f(0, 0, 1);
    const HitRecord medium_record = volume;
    context.expectTrue(!medium_record.isSurface(),
                       "variant carries the medium event type without fake surface fields");
    const Ray continuation = medium_record.spawnRay(Vec3f(0, 2, 0), 0.25f);
    context.expectVecNear(continuation.origin(), volume.point, 0.0f,
                          "volume continuation uses the event point without a normal offset");
    context.expectNear(continuation.time(), 0.25f, 0.0f, "continuation retains scene time");

}

}  // namespace


namespace {
class CountingShape final : public Shape {
public:
    mutable int calls = 0;
    bool hit(const Ray& ray, Interval interval, ShapeHit& hit) const override {
        ++calls;
        return sphere_.hit(ray, interval, hit);
    }
    const AABB& getBoundingBox() const override { return sphere_.getBoundingBox(); }
private:
    Sphere sphere_{Point3f(), 1};
};
class CountingTexture final : public Texture {
public:
    mutable int calls = 0;
    Color value(float, float, const Point3f&) const override {
        ++calls;
        return Color(0.5f, 0.5f, 0.5f);
    }
};
class CountingMethod final : public HitMethod {
public:
    mutable int calls = 0;
    bool supports(const Material& material) const override { return surface_.supports(material); }
    bool hit(const Shape& shape, const Material& material, const Ray& ray,
             Interval ray_t, float ray_length, HitRecord& record, Sampler& sampler) const override {
        ++calls;
        return surface_.hit(shape, material, ray, ray_t, ray_length, record, sampler);
    }
private:
    SurfaceIntersection surface_;
};
void testQueryWork(nearlighter::test::Context& context) {
    const auto shape = std::make_shared<CountingShape>();
    const auto texture = std::make_shared<CountingTexture>();
    const auto method = std::make_shared<CountingMethod>();
    const Primitive primitive(shape, std::make_shared<Lambertian>(texture), Transform(), method);
    const auto moved = std::make_shared<Primitive>(primitive.transformed(Transform::translate(Vec3f(0, 0, -3))));
    const Instance instance(moved, Transform::scale(Vec3f(2, 1, 1)));
    const BVH world{LinearAggregate(std::make_shared<Instance>(instance))};
    Sampler sampler(12), untouched(12);
    HitRecord record;
    context.expectTrue(world.hit(Ray(Point3f(), Vec3f(0, 0, -1)), Interval(0, infinity), record, sampler),
                       "instanced surface through BVH hits");
    context.expectTrue(shape->calls == 1 && method->calls == 1,
                       "BVH and Instance do not duplicate the leaf method or Shape query");
    context.expectTrue(texture->calls == 0, "candidate hit does not evaluate material textures");
    context.expectTrue(sampler.nextUInt32() == untouched.nextUInt32(), "surface query consumes no random sample");
    const auto response = record.material().scattering(record);
    context.expectTrue(std::holds_alternative<BSDF>(response) && texture->calls == 1,
                       "only final scattering construction resolves the texture");
    bool rejected = false;
    try {
        Primitive invalid(shape, std::make_shared<Homogeneous>(1, Color(1, 1, 1)), Transform(), method);
    } catch (const std::invalid_argument&) { rejected = true; }
    context.expectTrue(rejected, "incompatible material and hit method fail at construction");
}
}  // namespace

int main() {
    nearlighter::test::Context context;
    testShadingFrame(context);
    testSurfaceRecord(context);
    testQueryWork(context);
    return context.finish("interaction tests");
}
