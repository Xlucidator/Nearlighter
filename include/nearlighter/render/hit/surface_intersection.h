#ifndef NEARLIGHTER_RENDER_HIT_SURFACE_INTERSECTION_H
#define NEARLIGHTER_RENDER_HIT_SURFACE_INTERSECTION_H

#include <nearlighter/render/hit/hit_method.h>

/** Geometric Boundary as a Surface Event */
class SurfaceIntersection final : public HitMethod {
public:
    bool supports(const Material& material) const override;
    bool hit(const Shape& shape, const Material& material,
             const Ray& local_ray, Interval ray_t, float ray_length,
             HitRecord& record, Sampler& sampler) const override;
};

#endif  // NEARLIGHTER_RENDER_HIT_SURFACE_INTERSECTION_H
