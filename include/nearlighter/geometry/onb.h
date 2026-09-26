#ifndef NEARLIGHTER_GEOMETRY_ONB_H
#define NEARLIGHTER_GEOMETRY_ONB_H

#include <nearlighter/math/vec3.h>

/**
 * Right-Handed Orthonormal Basis
 *
 * Represents a right-handed basis `(u, v, w)` without an origin.
 * The `w` axis follows a supplied direction; all axes are expressed in the
 * coordinate system containing that direction.
 */
class ONB {
public:
    /**
     * Basis Construction
     *
     * @param w_direction Finite, non-zero direction for the local `w` axis.
     * @throws std::invalid_argument when the direction is zero or non-finite.
     */
    explicit ONB(const Vec3f& w_direction);

    /** Returns the basis axes expressed in the parent coordinate system. */
    const Vec3f& u() const { return u_; }
    const Vec3f& v() const { return v_; }
    const Vec3f& w() const { return w_; }

    /**
     * Local-to-Parent Direction Mapping
     *
     * Interprets `local` as coefficients along `(u, v, w)` and returns the
     * represented direction in the parent coordinate system.
     */
    Vec3f toParent(const Vec3f& local) const;

    /** Projects a parent-space direction into this orthonormal basis. */
    Vec3f toLocal(const Vec3f& parent) const;

private:
    Vec3f u_;
    Vec3f v_;
    Vec3f w_;
};

#endif  // NEARLIGHTER_GEOMETRY_ONB_H
