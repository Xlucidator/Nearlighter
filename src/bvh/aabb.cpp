#include "aabb.h"

// const AABB AABB::empty = AABB(Interval::empty, Interval::empty, Interval::empty);
// const AABB AABB::universe = AABB(Interval::universe, Interval::universe, Interval::universe);
const AABB& AABB::empty() {
    static AABB instance(Interval::empty, Interval::empty, Interval::empty);
    return instance;
}
const AABB& AABB::universe() {
    static AABB instance(Interval::universe, Interval::universe, Interval::universe);
    return instance;
}


AABB::AABB(const Point3f& p0, const Point3f& p1) {
    x = (p0.x() <= p1.x()) ? Interval(p0.x(), p1.x()) : Interval(p1.x(), p0.x());
    y = (p0.y() <= p1.y()) ? Interval(p0.y(), p1.y()) : Interval(p1.y(), p0.y());
    z = (p0.z() <= p1.z()) ? Interval(p0.z(), p1.z()) : Interval(p1.z(), p0.z());
    padding();
}

/* Generator version of uunion calculation
 */
AABB::AABB(const AABB& aabb0, const AABB& aabb1) {
    x = Interval(aabb0.x, aabb1.x);
    y = Interval(aabb0.y, aabb1.y);
    z = Interval(aabb0.z, aabb1.z);
}

int AABB::longestAxis() const {
    if (x.size() > y.size()) return x.size() > z.size() ? 0 : 2;
    else return y.size() > z.size() ? 1 : 2;
}

Point3f AABB::corner(int i) const {
#ifdef MORE_FLOAT_INSTRUCTIONS
    return Point3f(
        x.min + (i & 1) * (x.max - x.min),
        y.min + (i & 2) * (y.max - y.min),
        z.min + (i & 4) * (z.max - z.min)
    );
#else  // More Jump Instructions
    return Point3f(
        (i & 1) ? x.max : x.min,
        (i & 2) ? y.max : y.min,
        (i & 4) ? z.max : z.min
    );
#endif
}

void AABB::uunion(const AABB& other) {
    x.uunion(other.x);
    y.uunion(other.y);
    z.uunion(other.z);
}

bool AABB::hit(const Ray& ray, Interval ray_t) const {
    const Point3f& ray_origin = ray.origin();
    const Vec3f& ray_direction= ray.direction();

    for (int axis = 0; axis < 3; ++axis) {
        const Interval& axis_interval = getAxisInterval(axis);
        const float ray_orig_axis = ray_origin[axis];
        const float ray_dir_axis  = ray_direction[axis]; // TODO: compare with dir_axis_inv, see the error loss

        float t0 = (axis_interval.min - ray_orig_axis) / ray_dir_axis;
        float t1 = (axis_interval.max - ray_orig_axis) / ray_dir_axis;
        
        if (t0 > t1) std::swap(t0, t1);
        ray_t.min = std::max(ray_t.min, t0);
        ray_t.max = std::min(ray_t.max, t1);
        if (ray_t.is_null()) return false;
    }
    return true;
}


/* Private */

/* Make sure that no side of the AABB would be narrower than delta, padding if necessary
 *  This is to avoid the case where AABB is degenerated
 */
void AABB::padding() {
    constexpr float delta = 0.0001f, padding = delta * 0.5f;
    if (x.size() < delta) x.pad(padding);
    if (y.size() < delta) y.pad(padding);
    if (z.size() < delta) z.pad(padding);
}