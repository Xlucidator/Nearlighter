#include "test_support.h"

#include <nearlighter/accel/bvh.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/instance.h>
#include <nearlighter/scene/linear_aggregate.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/geometry/transform.h>

#include <array>
#include <memory>
#include <string>

namespace {

constexpr float kTolerance = 1e-5f;

std::shared_ptr<const Material> material() {
    static const auto value =
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f));
    return value;
}

std::shared_ptr<Primitive> spherePrimitive(const Point3f& center,
                                           float radius) {
    return std::make_shared<Primitive>(
        std::make_shared<Sphere>(center, radius), material());
}

LinearAggregate makeWorld() {
    LinearAggregate world;
    world.add(spherePrimitive(Point3f(-2.0f, 0.0f, -5.0f), 0.75f));
    world.add(spherePrimitive(Point3f(0.0f, 0.0f, -3.0f), 0.75f));
    world.add(spherePrimitive(Point3f(2.0f, 0.0f, -5.0f), 0.75f));
    world.add(spherePrimitive(Point3f(0.0f, -100.75f, -3.0f), 100.0f));
    return world;
}

void compareHit(nearlighter::test::Context& context,
                const LinearAggregate& world, const BVH& bvh,
                const Ray& ray, std::string_view ray_name) {
    HitRecord list_record;
    HitRecord bvh_record;
    const Interval ray_interval(0.001f, infinity);
    Sampler list_sampler(0);
    Sampler bvh_sampler(0);

    const bool list_hit = world.hit(ray, ray_interval, list_record,
                                    list_sampler);
    const bool bvh_hit = bvh.hit(ray, ray_interval, bvh_record, bvh_sampler);
    context.expectTrue(list_hit == bvh_hit,
                       std::string(ray_name) + " hit state");
    if (!list_hit || !bvh_hit) return;

    context.expectNear(bvh_record.t, list_record.t, kTolerance,
                       std::string(ray_name) + " hit distance");
    context.expectVecNear(bvh_record.point, list_record.point, kTolerance,
                          std::string(ray_name) + " hit point");
    context.expectVecNear(bvh_record.normal, list_record.normal, kTolerance,
                          std::string(ray_name) + " shading normal");
    context.expectTrue(bvh_record.front_face == list_record.front_face,
                       std::string(ray_name) + " face orientation");
    context.expectTrue(bvh_record.material == list_record.material,
                       std::string(ray_name) + " material binding");
}

void testInstances(nearlighter::test::Context& context) {
    LinearAggregate source_objects;
    source_objects.add(spherePrimitive(Point3f(0.0f, 0.0f, 0.0f), 0.5f));
    auto source_bvh = std::make_shared<BVH>(source_objects);
    const Instance left(source_bvh,
                        Transform::translate(Vec3f(-2.0f, 0.0f, -3.0f)));
    const Instance right(source_bvh,
                         Transform::translate(Vec3f(2.0f, 0.0f, -3.0f)));

    Sampler sampler(0);
    HitRecord left_record;
    context.expectTrue(
        left.hit(Ray(Point3f(-2.0f, 0.0f, 0.0f),
                     Vec3f(0.0f, 0.0f, -1.0f)),
                 Interval(0.001f, infinity), left_record, sampler),
        "left Instance should hit the shared BVH");
    HitRecord right_record;
    context.expectTrue(
        right.hit(Ray(Point3f(2.0f, 0.0f, 0.0f),
                      Vec3f(0.0f, 0.0f, -1.0f)),
                  Interval(0.001f, infinity), right_record, sampler),
        "right Instance should hit the same shared BVH");
    context.expectVecNear(left.getBoundingBox().centroid(),
                          Point3f(-2.0f, 0.0f, -3.0f), kTolerance,
                          "left Instance world bounds");
    context.expectVecNear(right.getBoundingBox().centroid(),
                          Point3f(2.0f, 0.0f, -3.0f), kTolerance,
                          "right Instance world bounds");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    const LinearAggregate world = makeWorld();
    const BVH bvh(world);
    const std::array<Ray, 7> rays = {
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Ray(Point3f(-2.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Ray(Point3f(2.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Ray(Point3f(0.0f, 2.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(1.0f, 0.0f, 0.0f)),
        Ray(Point3f(0.0f, 0.0f, -3.0f), Vec3f(1.0f, 0.0f, 0.0f)),
        Ray(Point3f(0.0f, 1.0f, -3.0f), Vec3f(0.0f, -1.0f, 0.0f)),
    };
    for (std::size_t index = 0; index < rays.size(); ++index) {
        compareHit(context, world, bvh, rays[index],
                   "ray " + std::to_string(index));
    }
    testInstances(context);
    return context.finish("BVH tests");
}
