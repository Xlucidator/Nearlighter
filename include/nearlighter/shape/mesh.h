#ifndef NEARLIGHTER_SHAPE_MESH_H
#define NEARLIGHTER_SHAPE_MESH_H

#include <nearlighter/shape/shape.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class Triangle;

/**
 * Indexed Triangle Data
 *
 * Stores topology and vertex attributes shared by mesh I/O, Mesh, and indexed
 * Triangle views.
 * - `positions` and `triangles` must be non-empty.
 * - Populated optional arrays align one-to-one with `positions`.
 * - Every triangle index addresses all populated vertex arrays.
 */
struct MeshData {
    std::vector<Point3f> positions;
    std::vector<Vec3f> normals;
    std::vector<std::array<float, 2>> texture_coordinates;
    std::vector<std::array<std::uint32_t, 3>> triangles;
};

/**
 * Indexed Triangle Mesh
 *
 * Owns immutable MeshData and lightweight Triangle views. A private
 * local-space BVH accelerates intersection without introducing Material,
 * Transform, or world-space object responsibilities.
 *
 * Mesh does not currently expose a combined surface sampling distribution.
 */
class Mesh final : public Shape {
public:
    /**
     * Builds immutable indexed data and its private triangle-index BVH.
     *
     * @param data Indexed attributes transferred into immutable shared storage.
     * @param generate_normals Generates area-weighted vertex normals only when
     * the input normal array is empty. Generated normals follow face winding;
     * supplied normals are always preserved.
     * @throws std::invalid_argument for missing, inconsistent, out-of-range,
     * or degenerate geometry data.
     */
    explicit Mesh(MeshData data, bool generate_normals = false);
    ~Mesh() override;

    // Disable copying; allow exclusive ownership of the private BVH to move.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    bool hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const override;

    const AABB& getBoundingBox() const override { return bounding_box_; }

    /** Returns the immutable indexed buffers referenced by triangle views. */
    const MeshData& data() const { return *data_; }

private:
    struct TriangleBVHNode;

    /** Validates attribute alignment, indices, and non-degenerate faces. */
    static void validate(const MeshData& data);

    /** Generates normalized area-weighted vertex normals in place. */
    static void generateNormals(MeshData& data);

    // Triangle views retain this immutable storage through shared ownership.
    std::shared_ptr<const MeshData> data_;
    std::vector<Triangle> triangles_;

    // Nodes store indices into triangles_ rather than owning scene objects.
    std::unique_ptr<TriangleBVHNode> bvh_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_SHAPE_MESH_H
