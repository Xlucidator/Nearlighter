#ifndef LAMBERTIAN_H
#define LAMBERTIAN_H

#include <nearlighter/material/material.h>

#include <memory>

class Texture;

/** Textured Diffuse Material; Resolves Albedo Independently at Each Hit */
class Lambertian : public Material {
public:
    Lambertian(const Color& albedo);
    Lambertian(std::shared_ptr<Texture> tex);

    std::optional<BSDF> computeBSDF(
        const SurfaceInteraction& interaction,
        TransportMode mode) const override;
    bool supportsPathIntegrator() const override { return true; }

    // Legacy integrator hooks.
    bool scatter(const Ray& ray_in, const HitRecord& record,
                 ScatterRecord& s_record, Sampler& sampler) const override;

    float getScatterPDFValue(const Ray& ray_in, const HitRecord& record,
                             const Ray& ray_scattered) const override;

private:
    std::shared_ptr<Texture> texture;
};

/** Local Diffuse Reflection with Evaluated Albedo */
class LambertianBxDF : public BxDF {
public:
    explicit LambertianBxDF(Color albedo) : albedo_(albedo) {}

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming,
                   TransportMode mode) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming,
              TransportMode mode) const override;
    std::optional<LocalBxDFSample> sample(
        const Vec3f& outgoing, const Vec2f& direction_sample,
        TransportMode mode) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Diffuse;
    }

private:
    Color albedo_;
};

#endif // LAMBERTIAN_H
