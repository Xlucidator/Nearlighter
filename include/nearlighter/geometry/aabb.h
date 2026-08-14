#ifndef AABB_H
#define AABB_H

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>

#include <ostream>

/**
 * Axis-aligned bounding box used for conservative traversal rejection.
 * Bounds built from intervals or points pad thin axes for robust intersection.
 */
class AABB {
public:
    Interval x, y, z;

    AABB() = default; // Default intervals produce empty bounds.
    AABB(const Interval& x, const Interval& y, const Interval& z);
    AABB(const Point3f& p0, const Point3f& p1); // Creates bounds two unordered extreme points and pads thin axes.
    AABB(const AABB& aabb0, const AABB& aabb1); // union of two boxes, no padding

    /* Bounds Queries */
    /** Returns the interval for axis 0 (x), 1 (y), or 2 (z). */
    const Interval& getAxisInterval(int axis) const { return axis == 1 ? y : axis == 2 ? z : x; }
    int longestAxis() const;
    Point3f centroid() const { return Point3f(x.center(), y.center(), z.center()); }

    /** Selects one corner by an index in [0, 7]. */
    Point3f corner(int index) const;

    /* In-place Operations */
    void uunion(const AABB& other);
    void operator+=(const Vec3f& offset) { x += offset.x(), y += offset.y(), z += offset.z(); }

    /**
     * Tests the common ray interval of all three axis slabs.
     *
     * @param ray Ray in the same coordinate space as this box.
     * @param ray_t Accepted ray-parameter interval.
     * @return true when clipping leaves a positive-width interval.
     */
    bool hit(const Ray& ray, Interval ray_t) const;

    friend std::ostream& operator<<(std::ostream& os, const AABB& a);

private:
    /** Expands thin axes so planar bounds remain intersectable. */
    void padding();
};

/* Non-mutating Operations */
inline AABB uunion(const AABB& aabb0, const AABB& aabb1) { return AABB(aabb0, aabb1); }

inline AABB operator+(const AABB& bbox, const Vec3f& offset) { return AABB(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z()); }
inline AABB operator+(const Vec3f& offset, const AABB& bbox) { return bbox + offset; }

#endif // AABB_H
