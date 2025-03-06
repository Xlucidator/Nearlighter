#ifndef AABB_H
#define AABB_H

#include "common.h"

class AABB {
public:
    Interval x, y, z;
    // static const AABB empty, universe;

    AABB() = default; // Default AABB use default Interval
    AABB(const Interval& x, const Interval& y, const Interval& z):x(x), y(y), z(z) { padding(); }
    AABB(const Point3f& p0, const Point3f& p1); // Do not assume p0 < p1, but they are both extreme points
    AABB(const AABB& aabb0, const AABB& aabb1);

    /* Extreme */
    static const AABB& empty();
    static const AABB& universe();

    /* Get Attribute */
    const Interval& getAxisInterval(int n) const { return n == 1 ? y : n == 2 ? z : x; } // x=0, y=1, z=2
    int longestAxis() const;
    Point3f centroid() const { return Point3f(x.center(), y.center(), z.center()); }
    Point3f corner(int i) const; // n = 0 ~ 7

    /* Calculation */
    void uunion(const AABB& other);
    void operator+=(const Vec3f& offset) { x += offset.x(), y += offset.y(), z += offset.z(); }

    /* Function */
    bool hit(const Ray& ray, Interval ray_t) const;

    /* Debug */
    friend std::ostream& operator<<(std::ostream& os, const AABB& a) {
        return os << "(" << a.x << "-" << a.y << "-" << a.z << ")";
    }

private:
    void padding();
};

inline AABB uunion(const AABB& aabb0, const AABB& aabb1) { return AABB(aabb0, aabb1); }

inline AABB operator+(const AABB& bbox, const Vec3f& offset) { return AABB(bbox.x + offset.x(), bbox.y + offset.y(), bbox.z + offset.z()); }
inline AABB operator+(const Vec3f& offset, const AABB& bbox) { return bbox + offset; }

#endif // AABB_H
