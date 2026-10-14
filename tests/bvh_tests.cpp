#include "test_support.h"


#include <nearlighter/accel/bvh.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/object/instance.h>
#include <nearlighter/scene/object/linear_aggregate.h>
#include <nearlighter/scene/object/primitive.h>
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

    context.expectNear(bvh_record.surface().t, list_record.surface().t, kTolerance,
                       std::string(ray_name) + " hit distance");
    context.expectVecNear(bvh_record.surface().point, list_record.surface().point, kTolerance,
                          std::string(ray_name) + " hit point");
    context.expectVecNear(bvh_record.surface().normal, list_record.surface().normal, kTolerance,
                          std::string(ray_name) + " shading normal");
    context.expectTrue(bvh_record.surface().front_face == list_record.surface().front_face,
                       std::string(ray_name) + " face orientation");
    context.expectTrue(bvh_record.surface().material == list_record.surface().material,
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


namespace {
/** Deterministic Events Isolate Record Selection from Random-Stream Ordering. */
class FixedEvent final : public Intersectable {
public:
    FixedEvent(float t, bool medium) : t_(t), medium_(medium), bounds_(Point3f(-1, -1, -10), Point3f(1, 1, 1)) {}
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record, Sampler&) const override {
        if (!ray_t.contains(t_)) return false;
        if (medium_) {
            MediumRecord event;
            event.t = t_;
            event.point = ray.at(t_);
            record = event;
        } else {
            SurfaceRecord event;
            event.t = t_;
            event.point = ray.at(t_);
            record = event;
        }
        return true;
    }
    const AABB& getBoundingBox() const override { return bounds_; }
private:
    float t_;
    bool medium_;
    AABB bounds_;
};

void testMixedRecords(nearlighter::test::Context& context) {
    const Ray ray(Point3f(), Vec3f(0, 0, -1));
    for (bool nearest_medium : {false, true}) {
        LinearAggregate objects;
        objects.add(std::make_shared<FixedEvent>(5.0f, !nearest_medium));
        objects.add(std::make_shared<FixedEvent>(2.0f, nearest_medium));
        const BVH bvh(objects);
        for (const Intersectable* world : {static_cast<const Intersectable*>(&objects),
                                           static_cast<const Intersectable*>(&bvh)}) {
            Sampler sampler(0);
            HitRecord record;
            context.expectTrue(world->hit(ray, Interval(0, 10), record, sampler), "mixed leaf query hits");
            context.expectNear(record.t(), 2, 0, "aggregate chooses the nearest event independent of kind");
            context.expectTrue(!record.isSurface() == nearest_medium,
                               "aggregate copies the selected variant alternative");
            context.expectFalse(world->hit(ray, Interval(0, 1), record, sampler), "clipped query misses");
            context.expectNear(record.t(), 2, 0, "aggregate miss leaves output unchanged");
            context.expectTrue(world->hit(ray, Interval(3, 10), record, sampler), "lower bound selects farther event");
            context.expectNear(record.t(), 5, 0, "lower clipping respects both event types");
        }
    }
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
    testMixedRecords(context);
    return context.finish("BVH tests");
}
