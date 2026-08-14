#ifndef NEARLIGHTER_GEOMETRY_TRANSFORM_H
#define NEARLIGHTER_GEOMETRY_TRANSFORM_H

#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>
#include <nearlighter/math/mat4.h>

/**
 * Affine Transformation
 *
 * - stores an invertible affine transformation and its inverse.
 * - the forward matrix has the form:
 *     M = [ A  t ]
 *         [ 0  1 ]
 *   where A is in GL(3, R) and t is in R^3.
 *   These matrices represent Aff(3, R), embedded in GL(4, R).
 *
 * apply*() uses M; applyInverse*() uses M^-1.
 * Ray directions are not normalized, so parameter t remains unchanged.
 */
class Transform {
public:
    Transform() = default; // Identity mapping.
    explicit Transform(const Mat4f& matrix);

    /**
     * @name Common Mappings
     * Rotation requires a non-zero axis and uses radians. Scale factors must
     * be numerically non-zero.
     * @{
     */
    static Transform translate(const Vec3f& offset);
    static Transform rotate(const Vec3f& axis, float angle_radians);
    static Transform scale(const Vec3f& factors);
    static Transform scale(float factor);
    /** @} */

    /** Composes transforms so (a * b)(p) applies b before a. */
    friend Transform operator*(const Transform& a, const Transform& b);

    /**
     * @name Forward Mapping
     * Maps geometric values through the forward matrix. Normals use its
     * inverse transpose and are returned normalized; orientation reversal is
     * reported separately.
     * @{
     */
    Point3f applyPoint(const Point3f& point) const;
    Vec3f applyVector(const Vec3f& vector) const;
    Vec3f applyNormal(const Vec3f& normal) const;
    Ray applyRay(const Ray& ray) const;
    AABB applyBounds(const AABB& bounds) const;
    /** @} */

    /**
     * @name Inverse Mapping
     * Maps points, vectors, and rays through the inverse matrix.
     * @{
     */
    Point3f applyInversePoint(const Point3f& point) const;
    Vec3f applyInverseVector(const Vec3f& vector) const;
    Ray applyInverseRay(const Ray& ray) const;
    /** @} */

    /**
     * Jacobian for Direction Mapping
     *
     * The factor for the change of a solid angle under this spatial transformation.
     * @return J = d(omega_input) / d(omega_output) for an output-space direction.
     * A zero-length direction returns zero; direction length is otherwise irrelevant.
     */
    float directionPDFJacobian(const Vec3f& output_direction) const;

    /** True when the forward linear map has a negative determinant. */
    bool isOrientationReversing() const {
        return is_orientation_reversing_;
    }

    bool isIdentity() const { return is_identity_; }

private:
    Transform(const Mat4f& matrix, const Mat4f& inverse_matrix);

    Mat4f matrix_;
    Mat4f inverse_matrix_;
    float inverse_linear_abs_determinant_ = 1.0f;  // |det| of inverse_matrix_'s upper-left 3x3 block.
    bool is_orientation_reversing_ = false;
    bool is_identity_ = true;
};

#endif  // NEARLIGHTER_GEOMETRY_TRANSFORM_H
