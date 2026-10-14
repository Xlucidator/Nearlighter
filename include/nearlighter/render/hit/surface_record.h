#ifndef NEARLIGHTER_RENDER_HIT_SURFACE_RECORD_H
#define NEARLIGHTER_RENDER_HIT_SURFACE_RECORD_H

#include <nearlighter/base/ray.h>

class Material;
class Primitive;

/**
 * Surface Event in the Queried Object's Coordinate Space
 *
 * Valid after a successful Primitive or scene query:
 * - Normals are unit vectors facing against the incident ray.
 * - outgoing is the unit direction toward the previous path vertex.
 * - Material and Primitive pointers borrow from the hit object's lifetime.
 * - Default construction alone does not represent a hit.
 */
struct SurfaceRecord {
    Point3f point;
    Vec3f geometric_normal;
    Vec3f normal;
    Vec3f outgoing;
    const Material* material = nullptr;
    const Primitive* primitive = nullptr;
    float t = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    bool front_face = false;

    /** Face-Forward Orientation from Compatible Unit Outward Normals */
    void setFaceNormals(const Ray& ray, const Vec3f& outward_geometric,
                        const Vec3f& outward_shading) {
        front_face = dot(ray.direction(), outward_geometric) < 0.0f;
        geometric_normal = front_face ? outward_geometric : -outward_geometric;
        normal = front_face ? outward_shading : -outward_shading;
    }

    Vec3f outwardGeometricNormal() const {
        return front_face ? geometric_normal : -geometric_normal;
    }

    /**
     * Continuation Ray from the Direction-Compatible Surface Side
     *
     * @param direction Finite non-zero direction; its length is preserved.
     * @param time Scene time retained by the path.
     */
    Ray spawnRay(const Vec3f& direction, float time) const;
};

#endif  // NEARLIGHTER_RENDER_HIT_SURFACE_RECORD_H
