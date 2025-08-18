#ifndef ONB_H
#define ONB_H

#include "vec3f.h"

class ONB {
public:
    ONB(const Vec3f& n) {
        axis[2] = unit_vector(n);
        Vec3f a = (std::fabs(n.x()) > 0.9) ? Vec3f(0, 1, 0) : Vec3f(1, 0, 1);
        axis[1] = unit_vector(cross(axis[2], a));
        axis[0] = cross(axis[1], axis[2]); // this is right-hand coordinates
    }

    const Vec3f& u() const { return axis[0]; }
    const Vec3f& v() const { return axis[1]; }
    const Vec3f& w() const { return axis[2]; }

    /**
     * Transform coordinates from local onb to standard (i, j, k) basis
     */
    Vec3f transform(const Vec3f& v) const {
        return v[0] * axis[0] + v[1] * axis[1] + v[2] * axis[2];
    }

private:
    Vec3f axis[3];
};

#endif // ONB_H