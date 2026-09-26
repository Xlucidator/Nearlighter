#ifndef NEARLIGHTER_SCENE_SURFACE_INTERACTION_H
#define NEARLIGHTER_SCENE_SURFACE_INTERACTION_H

#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/shading_frame.h>

class Material;
class Primitive;
struct HitRecord;

/** Complete world-space context for scattering at one surface point. */
class SurfaceInteraction {
public:
    /**
     * Builds a surface context from a completed intersection record.
     *
     * @param incident_ray Ray that produced the surface record.
     * @param record Surface record with non-null Material and Primitive.
     * @throws std::invalid_argument when record does not describe a complete
     * surface interaction.
     */
    SurfaceInteraction(const Ray& incident_ray, const HitRecord& record);

    const Point3f& point() const { return point_; }
    const Vec3f& geometricNormal() const { return geometric_normal_; }
    Vec3f outwardGeometricNormal() const {
        return front_face_ ? geometric_normal_ : -geometric_normal_;
    }
    const Vec3f& shadingNormal() const { return shading_normal_; }
    const Vec3f& outgoing() const { return outgoing_; }
    float rayParameter() const { return ray_parameter_; }
    float u() const { return u_; }
    float v() const { return v_; }
    bool frontFace() const { return front_face_; }
    const Primitive& primitive() const { return *primitive_; }
    const Material& material() const { return *material_; }
    const ShadingFrame& frame() const { return frame_; }

    /**
     * Spawns a ray on the direction-compatible side of the geometric surface.
     *
     * `direction` must be finite and non-zero. The returned direction is not
     * normalized, preserving the caller's parameterization.
     */
    Ray spawnRay(const Vec3f& direction, float time) const;

private:
    Point3f point_;
    Vec3f geometric_normal_;
    Vec3f shading_normal_;
    Vec3f outgoing_;
    float ray_parameter_ = 0.0f;
    float u_ = 0.0f;
    float v_ = 0.0f;
    bool front_face_ = false;
    const Primitive* primitive_ = nullptr;
    const Material* material_ = nullptr;
    ShadingFrame frame_;
};

#endif  // NEARLIGHTER_SCENE_SURFACE_INTERACTION_H
