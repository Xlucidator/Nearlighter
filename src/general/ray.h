#ifndef RAY_H
#define RAY_H

#include "vec3f.h"
#include "color.h"

class Ray {
public:
    float   tmin = 0;
    float   tmax = +infinity;

    Ray() {}
    Ray(const Point3f& origin, const Vec3f& direction):orig(origin), dir(direction) {}
    Ray(const Point3f& origin, const Vec3f& direction, float time):orig(origin), dir(direction), tm(time) {}

    inline const Point3f& origin() const { return orig; }
    inline const Vec3f& direction() const { return dir; }
    inline const Color& color() const { return colr; }
    inline const float time() const { return tm; }

    inline Point3f at(float t) const { return orig + t*dir; }
    inline bool in_inclusive_bound(const float t) const { return tmin <= t && t <= tmax; }
    inline bool in_exclusive_bound(const float t) const { return tmin < t  && t < tmax ; }

private:
    Point3f orig;
    Vec3f   dir;
    Color   colr;
    float   tm;     // represent the time when the ray was generated
};

#endif
