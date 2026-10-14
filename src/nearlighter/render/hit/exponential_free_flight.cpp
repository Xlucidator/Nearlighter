#include <nearlighter/render/hit/exponential_free_flight.h>
#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/shape/shape.h>

#include <algorithm>
#include <cmath>

bool ExponentialFreeFlight::supports(const Material& material) const {
    return dynamic_cast<const Homogeneous*>(&material) != nullptr;
}

/**
 * @par Implementation
 * The first two boundary intersections delimit one occupied segment.
 * Free-flight distance is s = -log(U) / extinction.
 * Input direction length converts physical distance to ray parameter t.
 */
bool ExponentialFreeFlight::hit(
    const Shape& shape, const Material& material, const Ray& local_ray,
    Interval ray_t, float ray_length, HitRecord& record, Sampler& sampler) const {
    /* ----- Boundary Segment ----- */
    ShapeHit entry, exit;
    if (!shape.hit(local_ray, Interval::universe, entry) ||
        !shape.hit(local_ray, Interval(entry.t + 0.0001f, infinity), exit)) {
        return false;
    }
    // Clipping also handles a ray whose origin is already inside the volume.
    const float begin = std::max({entry.t, ray_t.min, 0.0f});
    const float end = std::min(exit.t, ray_t.max);
    if (begin >= end || !(ray_length > 0.0f)) return false;

    /* ----- Free-Flight Distance ----- */
    const auto& medium = static_cast<const Homogeneous&>(material);
    // Keep log(U) finite without changing the sampler's endpoint convention.
    const float u = std::max(sampler.next1D(), 1e-7f);
    const float distance = -std::log(u) / medium.extinction();
    if (distance > (end - begin) * ray_length) return false;

    /* ----- Local Volume Event ----- */
    MediumRecord event;
    event.t = begin + distance / ray_length;
    event.point = local_ray.at(event.t);
    record = event;
    return true;
}
