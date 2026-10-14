#include <nearlighter/sampling/pdf.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/object/primitive.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

bool validDirection(const Vec3f& direction) {
    return std::isfinite(direction.x()) && std::isfinite(direction.y()) &&
           std::isfinite(direction.z()) &&
           (direction.x() != 0.0f || direction.y() != 0.0f || direction.z() != 0.0f);
}

Vec3f unitDirection(const Vec3f& direction) {
    // Rescale before squaring so finite direction magnitudes cannot overflow or underflow.
    const float scale = std::max({std::fabs(direction.x()), std::fabs(direction.y()),
                                  std::fabs(direction.z())});
    // Divide components directly because the reciprocal of a subnormal scale can overflow.
    const Vec3f scaled(direction.x() / scale, direction.y() / scale, direction.z() / scale);
    return unit_vector(scaled);
}

}  // namespace

// ==================================================
// Basic Angular Distributions
// ==================================================

float SpherePDF::value(const Vec3f& direction) const {
    return validDirection(direction) ? 1.0f / (4.0f * pi) : 0.0f;
}

Vec3f SpherePDF::generate(Sampler& sampler) const {
    // Uniform height and azimuth give equal probability per unit solid angle.
    const Vec2f u = sampler.next2D();
    const float z = 1.0f - 2.0f * u.x();
    const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
    const float phi = 2.0f * pi * u.y();
    return Vec3f(radius * std::cos(phi), radius * std::sin(phi), z);
}

float CosineHemispherePDF::value(const Vec3f& direction) const {
    if (!validDirection(direction)) return 0.0f;
    const float cosine = dot(frame_.normal(), unitDirection(direction));
    return std::max(0.0f, cosine) / pi;
}

Vec3f CosineHemispherePDF::generate(Sampler& sampler) const {
    // A uniform disk radius sqrt(u) projects to a cosine-weighted hemisphere.
    const Vec2f u = sampler.next2D();
    const float phi = 2.0f * pi * u.x();
    const float radius = std::sqrt(u.y());
    const Vec3f local(radius * std::cos(phi), radius * std::sin(phi),
                      std::sqrt(std::max(0.0f, 1.0f - u.y())));
    return frame_.toWorld(local);
}

// ==================================================
// Surface Targets and Distribution Composition
// ==================================================

SurfacePDF::SurfacePDF(const std::vector<std::shared_ptr<const Primitive>>& targets,
                       const Point3f& origin)
    : targets_(targets), origin_(origin) {}

float SurfacePDF::value(const Vec3f& direction) const {
    if (targets_.empty() || !validDirection(direction)) return 0.0f;
    // A canonical direction keeps geometric ray thresholds independent of input magnitude.
    const Vec3f incoming = unitDirection(direction);
    const float weight = 1.0f / static_cast<float>(targets_.size());
    float density = 0.0f;
    for (const auto& target : targets_) {
        density += weight * target->getPDFValue(origin_, incoming);
    }
    return density;
}

Vec3f SurfacePDF::generate(Sampler& sampler) const {
    if (targets_.empty()) return Vec3f();
    const int index = sampler.nextInt(0, static_cast<int>(targets_.size()) - 1);
    return targets_[static_cast<std::size_t>(index)]->random(origin_, sampler);
}

MixturePDF::MixturePDF(const PDF& first, const PDF& second, float first_probability)
    : first_(first), second_(second), first_probability_(first_probability) {
    if (!(first_probability >= 0.0f && first_probability <= 1.0f)) {
        throw std::invalid_argument("Mixture probability must lie in [0, 1]");
    }
}

float MixturePDF::value(const Vec3f& direction) const {
    if (first_probability_ == 1.0f) return first_.value(direction);
    if (first_probability_ == 0.0f) return second_.value(direction);
    return first_probability_ * first_.value(direction) +
           (1.0f - first_probability_) * second_.value(direction);
}

Vec3f MixturePDF::generate(Sampler& sampler) const {
    if (first_probability_ == 1.0f) return first_.generate(sampler);
    if (first_probability_ == 0.0f) return second_.generate(sampler);
    const PDF& component = sampler.next1D() < first_probability_ ? first_ : second_;
    return component.generate(sampler);
}
