#include <nearlighter/material/isotropic.h>

#include <nearlighter/sampling/pdf.h>
#include <nearlighter/texture/solid_texture.h>

Isotropic::Isotropic(shared_ptr<Texture> tex)
    : texture(tex) {}

Isotropic::Isotropic(const Color& albedo)
    : texture(make_shared<SolidTexture>(albedo)) {}

bool Isotropic::scatter(const Ray&, const HitRecord& record,
                        ScatterRecord& s_record, Sampler&) const {
    s_record.attenuation = texture->value(record.u, record.v, record.point);
    s_record.pdf = make_shared<SpherePDF>();
    s_record.should_skip = false;
    return true;
}

float Isotropic::getScatterPDF(const Ray&, const HitRecord&, const Ray&) const {
    return 1 / (4 * pi);
}
