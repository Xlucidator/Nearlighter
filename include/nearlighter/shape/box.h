#ifndef NEARLIGHTER_SHAPE_BOX_H
#define NEARLIGHTER_SHAPE_BOX_H

#include <nearlighter/shape/shape.h>

/**
 * Axis-Aligned Box
 *
 * Represents the six boundary faces of one non-degenerate local box without
 * allocating separate face Shapes.
 */
class Box final : public Shape {
public:
    /**
     * Creates a non-degenerate axis-aligned box in local coordinates.
     *
     * @throws std::invalid_argument unless maximum exceeds minimum on every
     * axis.
     */
    Box(const Point3f& minimum, const Point3f& maximum);

    bool hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const override;

    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return true; }

    /** Accounts for every box face reached by the query direction. */
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override;

    Vec3f random(const Point3f& origin, Sampler& sampler) const override;

private:
    /** Maps each face to its two increasing in-plane box axes. */
    void calculateUV(const Point3f& point, const Vec3f& normal,
                     float& u, float& v) const;

    Point3f minimum_;
    Point3f maximum_;
    Vec3f extent_;
    float surface_area_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_SHAPE_BOX_H
