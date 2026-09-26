#ifndef NEARLIGHTER_GEOMETRY_SHADING_FRAME_H
#define NEARLIGHTER_GEOMETRY_SHADING_FRAME_H

#include <nearlighter/geometry/onb.h>

/** Local orthonormal coordinates for surface-scattering evaluation. */
class ShadingFrame {
public:
    /** Builds a right-handed frame whose local z axis follows the normal. */
    explicit ShadingFrame(const Vec3f& shading_normal)
        : basis_(shading_normal) {}

    /** @name Frame Axes
     * Returns world-space tangent, bitangent, and shading-normal directions.
     * @{ */
    const Vec3f& tangent() const { return basis_.u(); }
    const Vec3f& bitangent() const { return basis_.v(); }
    const Vec3f& normal() const { return basis_.w(); }
    /** @} */

    /** Converts a world-space direction into this local frame. */
    Vec3f toLocal(const Vec3f& world) const { return basis_.toLocal(world); }

    /** Converts a local direction into world space. */
    Vec3f toWorld(const Vec3f& local) const { return basis_.toParent(local); }

private:
    ONB basis_;
};

#endif  // NEARLIGHTER_GEOMETRY_SHADING_FRAME_H
