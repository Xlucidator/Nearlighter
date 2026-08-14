#ifndef NEARLIGHTER_SHAPE_TRIANGLE_H
#define NEARLIGHTER_SHAPE_TRIANGLE_H

#include <nearlighter/shape/shape.h>

#include <cstddef>
#include <memory>

struct MeshData;

/**
 * Indexed Triangle
 *
 * Represents one two-sided triangle backed by immutable MeshData. Geometry is
 * flat; optional per-vertex normals and texture coordinates are interpolated.
 */
class Triangle final : public Shape {
public:
    /**
     * Creates a standalone triangle backed by an internal three-vertex mesh.
     *
     * @throws std::invalid_argument when the vertices are degenerate.
     */
    Triangle(const Point3f& p0, const Point3f& p1, const Point3f& p2);

    /**
     * Creates an indexed view into immutable MeshData without copying vertices.
     *
     * @param data Immutable buffers retained by shared ownership.
     * @param triangle_index Face index into MeshData::triangles.
     * @throws std::invalid_argument for null data or a degenerate face.
     * @throws std::out_of_range for an invalid face or vertex index.
     */
    Triangle(std::shared_ptr<const MeshData> data, std::size_t triangle_index);

    bool hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const override;

    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return true; }

    /** @name Uniform-Area Surface Sampling
     * @{ */
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override;
    Vec3f random(const Point3f& origin, Sampler& sampler) const override;
    /** @} */

private:
    std::shared_ptr<const MeshData> data_;
    std::size_t triangle_index_ = 0;
    Vec3f geometric_normal_;
    float area_ = 0.0f;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_SHAPE_TRIANGLE_H
