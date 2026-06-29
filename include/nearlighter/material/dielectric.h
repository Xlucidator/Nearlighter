#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include <nearlighter/material/material.h>

class Dielectric : public Material {
public:
    Dielectric(float refractive_index);

    bool scatter(const Ray& ray_in, const HitRecord& record, ScatterRecord& s_record) const override;

private:
    float refractive_index; // refractive index in vacuum, or index over the other enclosing media

    /* Fresnel reflectance (full version)
     *  r = (r_s + r_p ) / 2  non-polarized light
     *      r_s = [(eta_i cos_theta_i - eta_t cos_theta_t)(eta_i cos_theta_i + eta_t cos_theta_t)]^2
     *      r_p = [(eta_i cos_theta_t - eta_t cos_theta_i)(eta_i cos_theta_t + eta_t cos_theta_i)]^2
     */
    static float reflectance(float cos_theta_i, float cos_theta_t, float eta_i, float eta_t);

    /* Schilick's approximation for Fresnel reflectance 
     *  Reflectance -> r: the ratio of the ray_in to be reflected
     *      r = r0 + (1 - r0)(1 - cos_theta_i)^5
     *      r0: ratio when theta = 0; r0 = [(eta_i - eta_t)/(eta_i + eta_t)]^2
     *      eta_i <=> eta_1 input dielectic, eta_t <=> eta_2 output/refract dielectric
     */
    static float reflectance(float cos_theta_i, float eta_i, float eta_t);
};


#endif // DIELECTRIC_H
