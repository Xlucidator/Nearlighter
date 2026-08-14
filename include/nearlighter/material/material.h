#ifndef MATERIAL_H
#define MATERIAL_H

#include <nearlighter/scene/intersectable.h>
#include <nearlighter/texture/texture.h>

#include <memory>

class PDF;
class Sampler;

/** Material response used to continue one path at a surface interaction. */
struct ScatterRecord {
    Color attenuation;
    std::shared_ptr<PDF> sampling_pdf;
    bool should_skip;
    Ray skip_ray;
};

/** Polymorphic surface or phase response used by path integration. */
class Material {
public:
    virtual ~Material() = default;

    /**
     * Returns radiance emitted from an interaction toward the incoming ray.
     *
     * @param ray_in Incoming ray that reaches the interaction.
     * @param record Complete world-space interaction information.
     * @param u Horizontal texture coordinate.
     * @param v Vertical texture coordinate.
     * @param p World-space point used for texture evaluation.
     * @return Linear emitted radiance; the default is black.
     */
    virtual Color emitted(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] float u,
        [[maybe_unused]] float v,
        [[maybe_unused]] const Point3f& p
    ) const { return Color(0, 0, 0); }

    /**
     * Samples or describes the material response at an interaction.
     *
     * @param ray_in Incoming ray that reaches the interaction.
     * @param record Complete world-space interaction information.
     * @param s_record Receives attenuation and either a sampling PDF or a
     * directly selected delta-event ray when scattering occurs.
     * @param sampler Random stream consumed by stochastic responses.
     * @return true when the path should continue through a scattered event.
     */
    virtual bool scatter(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] ScatterRecord& s_record,
        [[maybe_unused]] Sampler& sampler
    ) const { return false; }

    /**
     * Evaluates material scattering density for a generated ray.
     *
     * @param ray_in Incoming ray that reaches the interaction.
     * @param record Complete world-space interaction information.
     * @param ray_scattered Candidate continuation ray.
     * @return Directional density with respect to solid angle.
     */
    virtual float getScatterPDFValue(
        [[maybe_unused]] const Ray& ray_in,
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] const Ray& ray_scattered
    ) const { return 0.0f; }

};

#endif // MATERIAL_H
