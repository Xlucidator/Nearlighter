#include <nearlighter/geometry/aabb.h>

#include <algorithm>

AABB::AABB(const Interval& x, const Interval& y, const Interval& z)
    : x(x), y(y), z(z) {
    padding();
}

AABB::AABB(const Point3f& p0, const Point3f& p1) {
    x = (p0.x() <= p1.x()) ? Interval(p0.x(), p1.x()) : Interval(p1.x(), p0.x());
    y = (p0.y() <= p1.y()) ? Interval(p0.y(), p1.y()) : Interval(p1.y(), p0.y());
    z = (p0.z() <= p1.z()) ? Interval(p0.z(), p1.z()) : Interval(p1.z(), p0.z());
    padding();
}

AABB::AABB(const AABB& aabb0, const AABB& aabb1) {
    x = Interval(aabb0.x, aabb1.x);
    y = Interval(aabb0.y, aabb1.y);
    z = Interval(aabb0.z, aabb1.z);
}

int AABB::longestAxis() const {
    if (x.size() > y.size()) return x.size() > z.size() ? 0 : 2;
    else return y.size() > z.size() ? 1 : 2;
}

Point3f AABB::corner(int index) const {
#ifdef MORE_FLOAT_INSTRUCTIONS
    return Point3f(
        x.min + (index & 1) * (x.max - x.min),
        y.min + (index & 2) * (y.max - y.min),
        z.min + (index & 4) * (z.max - z.min)
    );
#else  // More Jump Instructions
    return Point3f(
        (index & 1) ? x.max : x.min,
        (index & 2) ? y.max : y.min,
        (index & 4) ? z.max : z.min
    );
#endif
}

void AABB::uunion(const AABB& other) {
    x.uunion(other.x);
    y.uunion(other.y);
    z.uunion(other.z);
}

/**
 * @par Implementation
 * Intersects the ray interval with one slab at a time.
 * - Compute the entry and exit parameters on the current axis.
 * - Order them to support either ray direction.
 * - Clip the shared interval and reject it once empty.
 *
 * Zero direction components intentionally rely on IEEE-754 division:
 * - Infinite endpoints preserve or reject parallel rays by their origin.
 * - A boundary origin produces NaN and follows the comparison ordering.
 */
bool AABB::hit(const Ray& ray, Interval ray_t) const {
    const Point3f& ray_origin = ray.origin();
    const Vec3f& ray_direction = ray.direction();

    for (int axis = 0; axis < 3; ++axis) {
        const Interval& axis_interval = getAxisInterval(axis);
        const float ray_orig_axis = ray_origin[axis];
        const float ray_dir_axis = ray_direction[axis];

        float near_t = (axis_interval.min - ray_orig_axis) / ray_dir_axis;
        float far_t  = (axis_interval.max - ray_orig_axis) / ray_dir_axis;
        if (near_t > far_t) std::swap(near_t, far_t);

        ray_t.min = std::max(ray_t.min, near_t);
        ray_t.max = std::min(ray_t.max, far_t);
        if (ray_t.is_null()) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const AABB& a) {
    return os << "(" << a.x << "-" << a.y << "-" << a.z << ")";
}

void AABB::padding() {
    constexpr float delta = 0.0001f, padding = delta * 0.5f;
    if (x.size() < delta) x.pad(padding);
    if (y.size() < delta) y.pad(padding);
    if (z.size() < delta) z.pad(padding);
}
