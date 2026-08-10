#ifndef NEARLIGHTER_GEOMETRY_MESH_H
#define NEARLIGHTER_GEOMETRY_MESH_H

#include <nearlighter/geometry/shape.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class BVHNode;

/**
 * Indexed triangle data shared by mesh I/O and runtime geometry.
 *
 * Optional normals and texture coordinates are either empty or aligned
 * one-to-one with positions. Triangle indices always address positions.
 */
struct MeshData {
    std::vector<Point3f> positions;
    std::vector<Vec3f> normals;
    std::vector<std::array<float, 2>> texture_coordinates;
    std::vector<std::array<std::uint32_t, 3>> triangles;
};

/** Mesh construction behavior independent of its source file format. */
struct MeshBuildOptions {
    /** Generates area-weighted vertex normals only when input normals are absent. */
    bool generate_normals = false;
};

/** Indexed triangle aggregate accelerated by an internal BVH. */
class Mesh : public Shape {
public:
    /** Builds an internal BVH and optionally generates missing normals. */
    Mesh(MeshData data, std::shared_ptr<Material> material,
         const MeshBuildOptions& options = {});
    ~Mesh() override;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;

    bool hit(const Ray& ray, Interval ray_t, HitRecord& hit_record,
             Sampler& sampler) const override;
    const AABB& getBoundingBox() const override { return bounding_box_; }

    /** Returns immutable geometry shared by the internal Triangle objects. */
    const MeshData& data() const { return *data_; }

private:
    /**
     * Validates attribute layout, triangle indices, and non-degenerate faces.
     *
     * @throws std::invalid_argument when MeshData cannot safely form Triangles.
     */
    static void validate(const MeshData& data);

    /** Generates smooth area-weighted normals for all referenced vertices. */
    static void generateNormals(MeshData& data);

    /* Immutable indexed buffers shared by every Triangle view. */
    std::shared_ptr<const MeshData> data_;

    /* Triangle ownership, traversal acceleration, and cached aggregate bounds. */
    std::unique_ptr<BVHNode> bvh_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_GEOMETRY_MESH_H
