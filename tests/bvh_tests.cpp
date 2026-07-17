#include "test_support.h"

#include <nearlighter/accel/bvh_node.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/shape_list.h>
#include <nearlighter/geometry/sphere.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>

#include <array>
#include <memory>
#include <string>

namespace {

constexpr float kTolerance = 1e-5f;

ShapeList makeWorld() {
    ShapeList world;
    const std::shared_ptr<Material> material;
    world.add(std::make_shared<Sphere>(
        Point3f(-2.0f, 0.0f, -5.0f), 0.75f, material));
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -3.0f), 0.75f, material));
    world.add(std::make_shared<Sphere>(
        Point3f(2.0f, 0.0f, -5.0f), 0.75f, material));
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, -100.75f, -3.0f), 100.0f, material));
    return world;
}

void compareHit(nearlighter::test::Context& context,
                const ShapeList& world, const BVHNode& bvh,
                const Ray& ray, std::string_view ray_name) {
    // The linear list is the correctness reference. A BVH may reorder objects,
    // but it must preserve the closest-hit contract for every fixed ray.
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
                          std::string(ray_name) + " normal");
    context.expectTrue(bvh_record.front_face == list_record.front_face,
                       std::string(ray_name) + " face orientation");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    ShapeList world = makeWorld();
    BVHNode bvh(world);

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

    return context.finish("BVH tests");
}
