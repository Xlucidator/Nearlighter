#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/material/material.h>
#include <nearlighter/render/hit/surface_intersection.h>
#include <nearlighter/render/hit/exponential_free_flight.h>

#include <stdexcept>
#include <utility>

Primitive::Primitive(std::shared_ptr<const Shape> shape,
                     std::shared_ptr<const Material> material,
                     Transform local_to_parent,
                     std::shared_ptr<const HitMethod> hit_method)
    : shape_(std::move(shape)), material_(std::move(material)),
      hit_method_(std::move(hit_method)), local_to_parent_(std::move(local_to_parent)) {
    if (!shape_ || !material_) {
        throw std::invalid_argument("Primitive requires a Shape and Material");
    }
    if (!hit_method_) {
        // Default algorithms carry no Primitive data and can be shared read-only.
        static const auto surface = std::make_shared<const SurfaceIntersection>();
        static const auto medium = std::make_shared<const ExponentialFreeFlight>();
        if (material_->kind() == MaterialKind::Surface) hit_method_ = surface;
        else hit_method_ = medium;
    }
    if (!hit_method_->supports(*material_)) {
        throw std::invalid_argument("HitMethod is incompatible with the Primitive material");
    }
    bounds_ = local_to_parent_.applyBounds(shape_->getBoundingBox());
}

Primitive Primitive::transformed(const Transform& parent_to_world) const {
    return Primitive(shape_, material_, parent_to_world * local_to_parent_, hit_method_);
}

/**
 * @par Implementation
 * parent Ray -> Shape-local Ray -> local event -> bound parent event.
 * The inverse ray transform leaves direction unnormalized, preserving t.
 * Only the chosen method queries Shape; no preliminary geometry query is needed.
 */
bool Primitive::hit(const Ray& ray, Interval ray_t, HitRecord& record,
                    Sampler& sampler) const {
    /* ----- Local Event Query ----- */
    const Ray local_ray = local_to_parent_.isIdentity()
        ? ray : local_to_parent_.applyInverseRay(ray);
    if (!hit_method_->hit(*shape_, *material_, local_ray, ray_t,
                          ray.direction().length(), record, sampler)) return false;

    /* ----- Parent-Space Mapping and Binding ----- */
    record.transform(local_to_parent_, ray);
    record.bind(*material_, *this);
    return true;
}

bool Primitive::hasPDF() const {
    return material_->kind() == MaterialKind::Surface && shape_->hasPDF();
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
    if (!hasPDF()) return 0.0f;
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
