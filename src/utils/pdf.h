#ifndef PDF_H
#define PDF_H 

#include "common.h"
#include "shape/shape.h"

class PDF {
public:
    // virtual ~PDF() = default;
    virtual float value(const Vec3f& direction) const = 0;
    virtual Vec3f generate() const = 0;
};

class SpherePDF : public PDF {
public:
    SpherePDF() {}

    float value(const Vec3f& direction) const override {
        return 1 / (4 * pi);    
    }
    Vec3f generate() const override {
        return Vec3f::random_unit_vector();
    }
};

class CosineHemispherePDF : public PDF {
public:
    CosineHemispherePDF(const Vec3f& w) :uvw(w) {}

    float value(const Vec3f& direction) const override {
        float cosine_theta = dot(unit_vector(direction), uvw.w());
        return std::fmax(0, cosine_theta / pi);
    }
    Vec3f generate() const override {
        return uvw.transform(random_cosine_unit_on_hemisphere());
    }
private:
    ONB uvw;
};

class ShapePDF : public PDF {
public:
    ShapePDF(const Shape& objs, const Point3f& origin)
        : objects(objs), origin(origin) {}

    float value(const Vec3f& direction) const override {
        return objects.getPDFValue(origin, direction);
    }
    Vec3f generate() const override {
        return objects.random(origin);
    }
private:
    const Shape& objects;
    Point3f origin;
};

#endif // PDF_H