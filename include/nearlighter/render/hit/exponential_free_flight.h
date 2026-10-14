#ifndef NEARLIGHTER_RENDER_HIT_EXPONENTIAL_FREE_FLIGHT_H
#define NEARLIGHTER_RENDER_HIT_EXPONENTIAL_FREE_FLIGHT_H

#include <nearlighter/render/hit/hit_method.h>

/**
 * Exponential Event Distance inside a Closed Convex Shape
 *
 * Requires Homogeneous properties and exactly one interior segment per ray.
 * Sphere and Box satisfy this contract; arbitrary Mesh topology is not supported.
 */
class ExponentialFreeFlight final : public HitMethod {
public:
    bool supports(const Material& material) const override;
    bool hit(const Shape& shape, const Material& material,
             const Ray& local_ray, Interval ray_t, float ray_length,
             HitRecord& record, Sampler& sampler) const override;
};

#endif  // NEARLIGHTER_RENDER_HIT_EXPONENTIAL_FREE_FLIGHT_H
