#include <nearlighter/geometry/mesh.h>

#include <nearlighter/accel/bvh_node.h>
#include <nearlighter/geometry/triangle.h>

#include <stdexcept>
#include <utility>
#include <vector>

Mesh::Mesh(MeshData data, std::shared_ptr<Material> material,
           const MeshBuildOptions& options) {
    /* ----- Input invariants ----- */
    validate(data);

    /* ----- Shading attributes ----- */
    if (options.generate_normals && data.normals.empty()) {
        generateNormals(data);
    }

    /* ----- Shared geometry ----- */
    // One immutable allocation avoids copying indexed buffers into each face.
    data_ = std::make_shared<const MeshData>(std::move(data));

    /* ----- Triangle views ----- */
    std::vector<std::shared_ptr<Shape>> triangles;
    triangles.reserve(data_->triangles.size());
    for (std::size_t index = 0; index < data_->triangles.size(); ++index) {
        triangles.push_back(std::make_shared<Triangle>(data_, index, material));
    }

    /* ----- Internal acceleration ----- */
    // BVH nodes retain the Triangle objects after this temporary vector dies.
    bvh_ = std::make_unique<BVHNode>(triangles, 0, triangles.size());
    bounding_box_ = bvh_->getBoundingBox();
}

Mesh::~Mesh() = default;
Mesh::Mesh(Mesh&&) noexcept = default;
Mesh& Mesh::operator=(Mesh&&) noexcept = default;

/**
 * Intersects the Mesh through its internal Triangle BVH.
 *
 * BVH traversal rejects non-overlapping branches and delegates leaf tests to
 * Triangle::hit(), preserving the closest interaction inside ray_t.
 */
bool Mesh::hit(const Ray& ray, Interval ray_t, HitRecord& hit_record,
               Sampler& sampler) const {
    return bvh_->hit(ray, ray_t, hit_record, sampler);
}

void Mesh::validate(const MeshData& data) {
    /* ----- Required geometry ----- */
    if (data.positions.empty() || data.triangles.empty()) {
        throw std::invalid_argument("Mesh requires vertices and triangles");
    }

    /* ----- Attribute alignment ----- */
    // Triangle vertex indices address every populated per-vertex attribute.
    if (!data.normals.empty() && data.normals.size() != data.positions.size()) {
        throw std::invalid_argument(
            "Mesh normals must be empty or match the vertex count");
    }
    if (!data.texture_coordinates.empty() &&
        data.texture_coordinates.size() != data.positions.size()) {
        throw std::invalid_argument(
            "Mesh texture coordinates must be empty or match the vertex count");
    }

    /* ----- Triangle topology ----- */
    for (const auto& triangle : data.triangles) {
        // Reject invalid indices before any indexed position access.
        for (const std::uint32_t index : triangle) {
            if (index >= data.positions.size()) {
                throw std::invalid_argument("Mesh triangle index is out of range");
            }
        }
        const Vec3f edge_1 = data.positions[triangle[1]] -
                             data.positions[triangle[0]];
        const Vec3f edge_2 = data.positions[triangle[2]] -
                             data.positions[triangle[0]];
        // Zero-area faces have neither a stable normal nor a valid area PDF.
        if (cross(edge_1, edge_2).near_zero()) {
            throw std::invalid_argument("Mesh contains a degenerate triangle");
        }
    }
}

void Mesh::generateNormals(MeshData& data) {
    /* ----- Face accumulation ----- */
    data.normals.assign(data.positions.size(), Vec3f());
    for (const auto& triangle : data.triangles) {
        const Vec3f edge_1 = data.positions[triangle[1]] -
                             data.positions[triangle[0]];
        const Vec3f edge_2 = data.positions[triangle[2]] -
                             data.positions[triangle[0]];
        const Vec3f area_normal = cross(edge_1, edge_2);
        for (const std::uint32_t index : triangle) {
            // Cross-product magnitude is twice the face area, providing the
            // intended area weight before the final vertex normalization.
            data.normals[index] += area_normal;
        }
    }

    /* ----- Vertex normalization ----- */
    for (Vec3f& normal : data.normals) {
        // Unreferenced vertices do not participate in any Triangle and may
        // retain a zero normal without affecting intersection shading.
        if (!normal.near_zero()) normal = unit_vector(normal);
    }
}
