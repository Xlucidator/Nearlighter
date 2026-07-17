#include <nearlighter/material/metal.h>

#include <nearlighter/sampling/sampler.h>

Metal::Metal(const Color& albedo, float fuzz)
    : albedo(albedo), fuzz(fuzz < 1.0f ? fuzz : 1.0f) {}

bool Metal::scatter(const Ray& ray_in, const HitRecord& record,
                    ScatterRecord& s_record, Sampler& sampler) const {
    Vec3f reflect_direction = reflect(ray_in.direction(), record.normal);
    reflect_direction = unit_vector(reflect_direction) +
                        (fuzz * sampler.nextUnitVector());
    Ray scattered(record.point, reflect_direction, ray_in.time());

    s_record.attenuation = albedo;
    s_record.pdf = nullptr;
    s_record.should_skip = true;
    s_record.skip_ray = scattered;
    return dot(scattered.direction(), record.normal) > 0;
}
