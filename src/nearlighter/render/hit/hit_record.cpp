#include <nearlighter/render/hit/hit_record.h>
#include <nearlighter/geometry/transform.h>

#include <type_traits>

void HitRecord::transform(const Transform& local_to_parent, const Ray& parent_ray) {
    const bool identity = local_to_parent.isIdentity();
    std::visit([&](auto& event) {
        event.outgoing = -unit_vector(parent_ray.direction());
        if (identity) return;
        event.point = local_to_parent.applyPoint(event.point);
        if constexpr (std::is_same_v<std::decay_t<decltype(event)>, SurfaceRecord>) {
            // The source query already face-forwarded its normals.
            Vec3f geometric = local_to_parent.applyNormal(event.outwardGeometricNormal());
            Vec3f shading = local_to_parent.applyNormal(
                event.front_face ? event.normal : -event.normal);
            if (local_to_parent.isOrientationReversing()) {
                geometric = -geometric;
                shading = -shading;
            }
            if (dot(shading, geometric) < 0.0f) shading = -shading;
            event.setFaceNormals(parent_ray, geometric, shading);
        }
    }, value_);
}
