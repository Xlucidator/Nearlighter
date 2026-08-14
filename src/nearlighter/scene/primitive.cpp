#include <nearlighter/scene/primitive.h>

#include <nearlighter/sampling/sampler.h>

#include <stdexcept>
#include <utility>

Primitive::Primitive(std::shared_ptr<const Shape> shape,
                     std::shared_ptr<const Material> material,
                     Transform local_to_parent)
    : shape_(std::move(shape)),
      material_(std::move(material)),
      local_to_parent_(std::move(local_to_parent)) {
    if (!shape_) throw std::invalid_argument("Primitive requires a Shape");
    if (!material_) {
        throw std::invalid_argument("Primitive requires a Material");
    }
    bounds_ = local_to_parent_.applyBounds(shape_->getBoundingBox());
}

/**
 * @par Implementation
 * Performs the coordinate-space round trip required by `Shape::hit()`:
 *
 * `parent Ray -> Shape-local Ray -> ShapeHit -> parent HitRecord`
 *
 * 1. Apply the inverse placement to the ray. Its direction remains
 *    unnormalized, so the affine mapping preserves ray parameter `t`.
 * 2. Let Shape calculate geometry entirely in its local space.
 * 3. Apply the forward placement to the point and the inverse transpose to
 *    both outward normals. Negate the normals when the placement reverses the
 *    oriented-surface convention.
 * 4. Align the shading normal with the geometric normal. Then determine facing
 *    from the original parent-space ray and attach Material.
 *
 * Identity placement skips the coordinate conversions.
 */
bool Primitive::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                    Sampler&) const {
    /* ----- Local Geometry Query ----- */
    const bool identity_placement = local_to_parent_.isIdentity();
    const Ray local_ray = identity_placement
                              ? ray
                              : local_to_parent_.applyInverseRay(ray);
    ShapeHit local_hit;
    if (!shape_->hit(local_ray, ray_t, local_hit)) return false;

    /* ----- Parent-Space Interaction ----- */
    Vec3f outward_geometric = identity_placement
                                  ? local_hit.geometric_normal
                                  : local_to_parent_.applyNormal(
                                        local_hit.geometric_normal);
    Vec3f outward_shading = identity_placement
                                ? local_hit.shading_normal
                                : local_to_parent_.applyNormal(
                                      local_hit.shading_normal);

    // Keep geometric and shading orientations consistent under reflection.
    if (local_to_parent_.isOrientationReversing()) {
        outward_geometric = -outward_geometric;
        outward_shading = -outward_shading;
    }
    if (dot(outward_shading, outward_geometric) < 0.0f) {
        outward_shading = -outward_shading;
    }

    record.t = local_hit.t;
    record.point = identity_placement
                       ? local_hit.point
                       : local_to_parent_.applyPoint(local_hit.point);
    record.u = local_hit.u;
    record.v = local_hit.v;
    record.material = material_.get();
    record.setFaceNormals(ray, outward_geometric, outward_shading);
    return true;
}

/**
 * @par Implementation
 * - Map the origin and direction into Shape space.
 * - Evaluate the Shape's local solid-angle density.
 * - Convert that density with the direction-map Jacobian.
 *
 * The Jacobian is required under non-uniform scale.
 */
float Primitive::getPDFValue(const Point3f& origin,
                             const Vec3f& direction) const {
    if (!shape_->hasPDF()) return 0.0f;
    if (local_to_parent_.isIdentity()) {
        return shape_->getPDFValue(origin, direction);
    }

    const Point3f local_origin = local_to_parent_.applyInversePoint(origin);
    const Vec3f local_direction =
        local_to_parent_.applyInverseVector(direction);
    return shape_->getPDFValue(local_origin, local_direction) *
           local_to_parent_.directionPDFJacobian(direction);
}

Vec3f Primitive::random(const Point3f& origin, Sampler& sampler) const {
    if (local_to_parent_.isIdentity()) {
        return shape_->random(origin, sampler);
    }

    // Translation affects the local origin but not the returned direction.
    const Point3f local_origin = local_to_parent_.applyInversePoint(origin);
    return local_to_parent_.applyVector(shape_->random(local_origin, sampler));
}
