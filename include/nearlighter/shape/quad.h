#ifndef NEARLIGHTER_SHAPE_QUAD_H
#define NEARLIGHTER_SHAPE_QUAD_H

#include <nearlighter/shape/shape.h>

/**
 * Finite Parallelogram
 *
 * Represents `origin + a*u + b*v` for `a,b` in `[0,1]`.
 */
class Quad final : public Shape {
public:
    /**
     * Creates a local parallelogram spanning origin + a*u + b*v.
     *
     * @throws std::invalid_argument when the edge vectors are degenerate.
     */
    Quad(const Point3f& origin, const Vec3f& u, const Vec3f& v);

    bool hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const override;

    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return true; }

    /** @name Uniform-Area Surface Sampling
     * @{ */
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override;
    Vec3f random(const Point3f& origin, Sampler& sampler) const override;
    /** @} */

private:
    static AABB calculateAABB(const Point3f& origin, const Vec3f& u,
                              const Vec3f& v);

    Point3f origin_;
    Vec3f u_;
    Vec3f v_;
    Vec3f normal_;
    Vec3f w_;             // Recovers coordinates in the non-orthogonal basis.
    float plane_offset_;  // `normal dot point` for the supporting plane.
    float area_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_SHAPE_QUAD_H
