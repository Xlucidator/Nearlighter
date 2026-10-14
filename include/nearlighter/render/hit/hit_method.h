#ifndef NEARLIGHTER_RENDER_HIT_HIT_METHOD_H
#define NEARLIGHTER_RENDER_HIT_HIT_METHOD_H

#include <nearlighter/base/interval.h>
#include <nearlighter/material/material.h>
#include <nearlighter/render/hit/hit_record.h>

class Shape;
class Sampler;

/** Single-Primitive Event Location; No Direction Scattering or Scene Traversal */
class HitMethod {
public:
    virtual ~HitMethod() = default;

    /** Construction-Time Material Compatibility */
    virtual bool supports(const Material& material) const = 0;

    /**
     * Local Event Query
     *
     * @param local_ray Shape-local ray; its affine parameter t is unchanged.
     * @param ray_t Accepted parameter interval.
     * @param ray_length Length of Primitive's input direction, before its transform.
     * @param record Receives an unbound local event; miss leaves it unchanged.
     * @param sampler Random source for stochastic event location.
     * The event alternative must match the supported material family.
     * Primitive completes the binding, outgoing direction, and coordinate mapping.
     */
    virtual bool hit(const Shape& shape, const Material& material,
                     const Ray& local_ray, Interval ray_t, float ray_length,
                     HitRecord& record, Sampler& sampler) const = 0;
};

#endif  // NEARLIGHTER_RENDER_HIT_HIT_METHOD_H
