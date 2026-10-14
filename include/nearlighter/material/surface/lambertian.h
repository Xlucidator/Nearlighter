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

    MaterialKind kind() const override { return MaterialKind::Surface; }
    ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const override;

private:
    std::shared_ptr<Texture> texture;
};

/** Local Diffuse Reflection with Evaluated Albedo */
class LambertianBxDF : public BxDF {
public:
    explicit LambertianBxDF(Color albedo) : albedo_(albedo) {}

    Color evaluate(const Vec3f& outgoing, const Vec3f& incoming) const override;
    float PDF(const Vec3f& outgoing, const Vec3f& incoming) const override;
    std::optional<ScatteringSample> sample(
        const Vec3f& outgoing, Sampler& sampler) const override;
    BxDFFlags flags() const override {
        return BxDFFlags::Reflection | BxDFFlags::Diffuse;
    }

private:
    Color albedo_;
};

#endif // LAMBERTIAN_H
