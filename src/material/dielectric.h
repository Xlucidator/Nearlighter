#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include "material.h"

class Dielectric : public Material {
public:
    Dielectric(float refractive_index) : refractive_index(refractive_index) {}

    bool scatter(const Ray& ray_in, const HitRecord& record, Color& attenuation, Ray& scattered) const override {
        float etai_over_etao = record.front_face ? (1.0f / refractive_index) : refractive_index;
  
        Vec3f direction;
        Vec3f unit_ray_in = unit_vector(ray_in.direction());
        float cos_theta_i = std::fmin(dot(-unit_ray_in, record.normal), 1.0f);
        float sin_theta_i = std::sqrt(1.0f - cos_theta_i * cos_theta_i);

        /* Consider Total Internal Reflection */
        // etai * in_sin_theta = etao * out_sin_theta 
        // => sin_theta_t = etai_over_etao * sin_theta_i
        bool total_internal_reflection = (etai_over_etao * sin_theta_i) > 1.0f;

        /* Consider Fresnel Reflectance */
        bool fresnel_reflect = reflectance(cos_theta_i, 1.0f, etai_over_etao) > random_float(); // convert ratio -> probability

        if (total_internal_reflection || fresnel_reflect) { // total internal reflection
            direction = reflect(unit_ray_in, record.normal);
        } else {
            direction = refract(unit_ray_in, record.normal, etai_over_etao);
        }

        scattered = Ray(record.point, direction);
        attenuation = Color(1.0f, 1.0f, 1.0f);
        return true;
    }

private:
    float refractive_index; // refractive index in vacuum, or index over the other enclosing media

    /* Fresnel reflectance (full version)
     *  r = (r_s + r_p ) / 2  non-polarized light
     *      r_s = [(eta_1 cos_theta_i - eta_2 cos_theta_t)(eta_1 cos_theta_i + eta_2 cos_theta_t)]^2
     *      r_p = [(eta_1 cos_theta_t - eta_2 cos_theta_i)(eta_1 cos_theta_t + eta_2 cos_theta_i)]^2
     */
    static float reflectance(float cos_theta_i, float cos_theta_t, float eta_1, float eta_2) {
        
        return 0.0;
    }

    /* Schilick's approximation for Fresnel reflectance 
     *  Reflectance -> r: the ratio of the ray_in to be reflected
     *      r = r0 + (1 - r0)(1 - cos_theta_i)^5
     *      r0: ratio when theta = 0; r0 = [(eta_1 - eta_2)/(eta_1 + eta_2)]^2
     */
    static float reflectance(float cos_theta_i, float eta_1, float eta_2) {
        float r0 = (eta_1 - eta_2) / (eta_1 + eta_2);
        r0 *= r0;
        return r0 + (1 - r0) * std::pow((1 - cos_theta_i), 5);
    }
};


#endif // DIELECTRIC_H
