#include "test_support.h"

#include <nearlighter/base/interval.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/shape/sphere.h>

#include <algorithm>
#include <cmath>
#include <memory>

int main() {
    nearlighter::test::Context context;
    const auto boundary = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, 0.0f), 1.0f),
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f)));
    constexpr float density = 100.0f;
    const ConstantMedium medium(boundary, density,
                                Color(0.2f, 0.4f, 0.8f));
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
    context.expectTrue(hit, "dense ConstantMedium should scatter");
    if (hit) {
        context.expectNear(record.t, expected_t, 1e-5f,
                           "ConstantMedium fixed-seed free-flight distance");
        context.expectVecNear(record.point, ray.at(expected_t), 1e-5f,
                              "ConstantMedium interaction point");
        context.expectTrue(record.material != nullptr,
                           "ConstantMedium should bind its phase Material");
        context.expectTrue(record.front_face,
                           "volume interactions use neutral facing state");
    }
    context.expectVecNear(medium.getBoundingBox().centroid(),
                          Point3f(0.0f, 0.0f, 0.0f), 1e-5f,
                          "ConstantMedium should reuse boundary bounds");
    return context.finish("ConstantMedium tests");
}
