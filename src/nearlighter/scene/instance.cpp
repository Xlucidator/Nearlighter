#include <nearlighter/scene/instance.h>

#include <stdexcept>
#include <utility>

Instance::Instance(std::shared_ptr<const Intersectable> source,
                   Transform local_to_parent)
    : source_(std::move(source)),
      local_to_parent_(std::move(local_to_parent)) {
    if (!source_) throw std::invalid_argument("Instance requires a source");
    bounds_ = local_to_parent_.applyBounds(source_->getBoundingBox());
}

/**
 * @par Implementation
 * Performs a coordinate-space round trip around the shared source:
 *
 * `parent Ray -> source Ray -> source HitRecord -> parent HitRecord`
 *
 * 1. Apply the inverse placement to the ray. Its direction remains
 *    unnormalized, so the affine mapping preserves ray parameter `t`.
 * 2. Query the source in its own coordinate space.
 * 3. Recover outward normals because the source HitRecord has already
 *    face-forwarded them against the source-space ray.
 * 4. Transform the point and outward normals into parent space. Correct an
 *    orientation reversal, then determine facing from the original ray.
 *
 * Ray parameter, UV coordinates, and Material pass through unchanged.
 *
 * Identity placement returns the source interaction directly.
 */
bool Instance::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                   Sampler& sampler) const {
    // Identity placement preserves the source record and skips matrix work.
    if (local_to_parent_.isIdentity()) {
        return source_->hit(ray, ray_t, record, sampler);
    }

    /* ----- Source-Space Query ----- */
    const Ray local_ray = local_to_parent_.applyInverseRay(ray);
    HitRecord local_record;
    if (!source_->hit(local_ray, ray_t, local_record, sampler)) return false;

    /* ----- Parent-Space Position ----- */
    record = local_record;
    record.point = local_to_parent_.applyPoint(local_record.point);
    if (local_record.kind == InteractionKind::Medium) return true;

    /* ----- Source Orientation Recovery ----- */
    const Vec3f local_outward_geometric =
        local_record.front_face ? local_record.geometric_normal
                                : -local_record.geometric_normal;
    const Vec3f local_outward_shading =
        local_record.front_face ? local_record.normal : -local_record.normal;
    Vec3f outward_geometric =
        local_to_parent_.applyNormal(local_outward_geometric);
    Vec3f outward_shading =
        local_to_parent_.applyNormal(local_outward_shading);

    /* ----- Parent-Space Interaction ----- */
    if (local_to_parent_.isOrientationReversing()) {
        outward_geometric = -outward_geometric;
        outward_shading = -outward_shading;
    }
    if (dot(outward_shading, outward_geometric) < 0.0f) {
        outward_shading = -outward_shading;
    }

    record.setFaceNormals(ray, outward_geometric, outward_shading);
    return true;
}
