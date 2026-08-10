#ifndef SHAPE_H
#define SHAPE_H

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>

#include <memory>

class Material;
class Sampler;

struct HitRecord {
    Point3f point;
    Vec3f normal;
    std::shared_ptr<Material> material;
    float t;
    float u, v;
    bool front_face;

    void set_face_normal(const Ray& r, const Vec3f& outward_normal) {
        // Normals are always oriented against the incident ray.
        // Assumption: outward_normal is already unit length.
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Shape {
public:
    virtual ~Shape() = default;

    /** Intersects the shape and records the closest accepted interaction. */
    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& hit_record,
                     Sampler& sampler) const = 0;

    /** Returns the shape's world-space axis-aligned bounding box. */
    virtual const AABB& getBoundingBox() const = 0;

    /**
     * Reports whether random() and getPDFValue() form a valid sampling pair.
     *
     * The renderer uses this geometric strategy to aim a path toward the
     * Shape, normally for explicit light sampling. It is independent of the
     * material BSDF. Do not call the default pair when this returns false.
     */
    virtual bool hasPDF() const { return false; }

    /**
     * Returns p(direction | origin) for the strategy implemented by random().
     *
     * The value is a probability density per unit solid angle, not the
     * probability of one exact direction. It is zero outside the strategy's
     * support; direction length does not affect the density.
     */
    virtual float getPDFValue(
        [[maybe_unused]] const Point3f& origin,
        [[maybe_unused]] const Vec3f& direction
    ) const {
        return 0.0f;
    }

    /**
     * Samples this Shape and returns a world-space direction from origin.
     *
     * The vector need not be normalized. The caller evaluates the same vector
     * with getPDFValue() when weighting the generated path sample.
     */
    virtual Vec3f random(
        [[maybe_unused]] const Point3f& origin,
        [[maybe_unused]] Sampler& sampler
    ) const {
        return Vec3f(1, 0, 0);
    }

    /* Debug */
    virtual void printNode([[maybe_unused]] int level) const {}
};

#endif
