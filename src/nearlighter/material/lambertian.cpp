#include <nearlighter/material/lambertian.h>

#include <nearlighter/sampling/pdf.h>
#include <nearlighter/texture/solid_texture.h>

Lambertian::Lambertian(const Color& albedo)
    : texture(std::make_shared<SolidTexture>(albedo)) {}

Lambertian::Lambertian(std::shared_ptr<Texture> tex)
    : texture(tex) {}

bool Lambertian::scatter(const Ray&, const HitRecord& record,
                         ScatterRecord& s_record, Sampler&) const {
    s_record.attenuation = texture->value(record.u, record.v, record.point);
    s_record.pdf = std::make_shared<CosineHemispherePDF>(record.normal);
    s_record.should_skip = false;
    return true;
}

float Lambertian::getScatterPDF(const Ray&, const HitRecord& record,
                                const Ray& ray_scattered) const {
    // scatterPDF(w_o) = cos(theta_o) / pi
    float cos_theta_o = dot(record.normal, unit_vector(ray_scattered.direction()));
    return cos_theta_o > 0.0f ? cos_theta_o / pi : 0.0f;
}
