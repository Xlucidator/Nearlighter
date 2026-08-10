#ifndef NEARLIGHTER_GEOMETRY_TRIANGLE_H
#define NEARLIGHTER_GEOMETRY_TRIANGLE_H

#include <nearlighter/geometry/mesh.h>
#include <nearlighter/geometry/shape.h>

#include <cstddef>
#include <memory>

/** One flat or vertex-normal-interpolated triangle primitive. */
class Triangle : public Shape {
public:
    /** Constructs a standalone triangle with a geometric normal. */
    Triangle(const Point3f& p0, const Point3f& p1, const Point3f& p2,
             std::shared_ptr<Material> material);

    /** References one indexed face in immutable shared Mesh data. */
    Triangle(std::shared_ptr<const MeshData> data, std::size_t triangle_index,
             std::shared_ptr<Material> material);

    bool hit(const Ray& ray, Interval ray_t, HitRecord& hit_record,
             Sampler& sampler) const override;
    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return true; }

    /** Evaluates the solid-angle PDF induced by uniform area sampling. */
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override;

    /** Samples one surface point uniformly and returns its direction. */
    Vec3f random(const Point3f& origin, Sampler& sampler) const override;

private:
    bool hitDeterministic(const Ray& ray, Interval ray_t,
                          HitRecord& hit_record) const;

    std::shared_ptr<const MeshData> data_;
    std::size_t triangle_index_ = 0;
    std::shared_ptr<Material> material_;
    Vec3f geometric_normal_;
    float area_ = 0.0f;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_GEOMETRY_TRIANGLE_H
