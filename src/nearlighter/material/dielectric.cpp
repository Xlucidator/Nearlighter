#include <nearlighter/material/dielectric.h>

#include <nearlighter/math/random.h>

#include <cmath>

Dielectric::Dielectric(float refractive_index)
    : refractive_index(refractive_index) {}

bool Dielectric::scatter(const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record) const {
    s_record.attenuation = Color(1.0f, 1.0f, 1.0f);
    s_record.pdf = nullptr;
    s_record.should_skip = true;

    float etai_over_etat = record.front_face ? (1.0f / refractive_index) : refractive_index;

    Vec3f unit_ray_in = unit_vector(ray_in.direction());
    float cos_theta_i = std::fmin(dot(-unit_ray_in, record.normal), 1.0f);
    float sin_theta_i = std::sqrt(1.0f - cos_theta_i * cos_theta_i);

    // Total internal reflection happens when Snell's law has no real transmitted direction.
    bool total_internal_reflection = (etai_over_etat * sin_theta_i) > 1.0f;

    // Schlick Fresnel reflectance is used as the probability of sampling reflection.
    bool fresnel_reflect = reflectance(cos_theta_i, etai_over_etat, 1.0f) > random_float();

    Vec3f direction;
    if (total_internal_reflection || fresnel_reflect) {
        direction = reflect(unit_ray_in, record.normal);
    } else {
        direction = refract(unit_ray_in, record.normal, etai_over_etat);
    }

    s_record.skip_ray = Ray(record.point, direction, ray_in.time());
    return true;
}

float Dielectric::reflectance(float, float, float, float) {
    return 0.0f;
}

float Dielectric::reflectance(float cos_theta_i, float eta_i, float eta_t) {
    float r0 = (eta_i - eta_t) / (eta_i + eta_t);
    r0 *= r0;
    return r0 + (1 - r0) * std::pow((1 - cos_theta_i), 5);
}
