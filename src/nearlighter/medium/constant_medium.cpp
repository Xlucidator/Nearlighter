#include <nearlighter/medium/constant_medium.h>

#include <nearlighter/material/isotropic.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {

float negativeInverseDensity(float density) {
    if (density <= 0.0f) {
        throw std::invalid_argument("ConstantMedium density must be positive");
    }
    return -1.0f / density;
}

}  // namespace

ConstantMedium::ConstantMedium(std::shared_ptr<const Primitive> boundary,
                               float density,
                               std::shared_ptr<Texture> texture)
    : boundary_(std::move(boundary)),
      negative_inverse_density_(negativeInverseDensity(density)),
      phase_function_(std::make_shared<Isotropic>(std::move(texture))) {
    if (!boundary_) {
        throw std::invalid_argument("ConstantMedium requires a boundary");
    }
}

ConstantMedium::ConstantMedium(std::shared_ptr<const Primitive> boundary,
                               float density,
                               const Color& albedo)
    : boundary_(std::move(boundary)),
      negative_inverse_density_(negativeInverseDensity(density)),
      phase_function_(std::make_shared<Isotropic>(albedo)) {
    if (!boundary_) {
        throw std::invalid_argument("ConstantMedium requires a boundary");
    }
}

/**
 * @par Implementation
 * - The first two boundary hits delimit the occupied ray segment.
 * - The free-flight distance follows `s = -log(U) / density`.
 * - Ray direction length converts between parameter and spatial distance.
 *
 * Different from ray marching, this method calculate distance at once.
 */
bool ConstantMedium::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                         Sampler& sampler) const {
    /* ----- Boundary Segment ----- */
    /*
     * A closed convex boundary has one entry and one exit along the complete
     * ray. Clipping them to ray_t also handles rays that begin inside it.
     */
    HitRecord entry;
    HitRecord exit;
    if (!boundary_->hit(ray, Interval::universe, entry, sampler)) {
        return false;
    }
    if (!boundary_->hit(ray, Interval(entry.t + 0.0001f, infinity), exit,
                        sampler)) {
        return false;
    }

    entry.t = std::fmax(entry.t, ray_t.min);
    exit.t = std::fmin(exit.t, ray_t.max);
    if (entry.t >= exit.t || exit.t < 0.0f) return false;
    entry.t = std::fmax(entry.t, 0.0f);

    /* ----- Free-Flight Distance ----- */
    // Ray directions are not normalized; |direction| converts t to distance.
    const float ray_length = ray.direction().length();
    if (ray_length <= 0.0f) return false;
    // Clamping U away from zero keeps -log(U) finite.
    const float random_value = std::max(sampler.next1D(), 1e-7f);
    const float scattering_distance =
        std::log(random_value) * negative_inverse_density_;
    const float segment_distance = (exit.t - entry.t) * ray_length;
    if (scattering_distance > segment_distance) return false;

    /* ----- Volume Interaction ----- */
    record.t = entry.t + scattering_distance / ray_length;
    record.point = ray.at(record.t);
    /*
     * A volume event has no surface orientation or UV parameterization.
     * Placeholders satisfy the shared HitRecord contract;
     * spatial textures still receive record.point through the phase Material.
     */
    record.geometric_normal = Vec3f(1.0f, 0.0f, 0.0f);
    record.normal = record.geometric_normal;
    record.front_face = true;
    record.u = 0.0f;
    record.v = 0.0f;
    record.material = phase_function_.get();
    return true;
}

const AABB& ConstantMedium::getBoundingBox() const {
    return boundary_->getBoundingBox();
}
