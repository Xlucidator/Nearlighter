#include <nearlighter/shape/mesh.h>

#include <nearlighter/shape/triangle.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>

struct Mesh::TriangleBVHNode {
    /**
     * Local Triangle Subtree Construction
     *
     * Builds one subtree over a non-empty half-open span of triangle indices.
     *
     * @par Implementation
     * Each node caches the span bounds. A single index becomes a leaf;
     * otherwise `nth_element` partitions centroid values at the median of the
     * longest bounds axis and both halves recurse.
     */
    TriangleBVHNode(std::vector<std::size_t>& indices, std::size_t begin,
                    std::size_t end,
                    const std::vector<Triangle>& triangles) {
        /* ----- Subtree Bounds ----- */
        // Nodes retain triangle indices only; placement and material remain
        // outside Mesh.
        for (std::size_t index = begin; index < end; ++index) {
            bounds.uunion(triangles[indices[index]].getBoundingBox());
        }

        /* ----- Single-Triangle Leaf ----- */
        const std::size_t count = end - begin;
        if (count == 1) {
            triangle_index = indices[begin];
            return;
        }

        /* ----- Centroid Median Partition ----- */
        // Reordering the temporary index array never changes MeshData topology
        // or the public order of indexed faces.
        const int axis = bounds.longestAxis();
        const std::size_t middle = begin + count / 2;
        std::nth_element(
            indices.begin() + begin, indices.begin() + middle,
            indices.begin() + end,
            [&](std::size_t a, std::size_t b) {
                return triangles[a].getBoundingBox().centroid()[axis] <
                       triangles[b].getBoundingBox().centroid()[axis];
            });
        left = std::make_unique<TriangleBVHNode>(indices, begin, middle,
                                                 triangles);
        right = std::make_unique<TriangleBVHNode>(indices, middle, end,
                                                  triangles);
    }

    /**
     * Local Triangle Traversal
     *
     * Returns the closest triangle interaction within `ray_t`.
     *
     * @par Implementation
     * Node bounds reject disjoint rays. After the left subtree hits, its `t`
     * becomes the right traversal's upper bound, so any accepted right hit is
     * necessarily closer.
     */
    bool hit(const Ray& ray, Interval ray_t, ShapeHit& record,
             const std::vector<Triangle>& triangles) const {
        /* ----- Bounds and Leaf Test ----- */
        if (!bounds.hit(ray, ray_t)) return false;
        if (triangle_index != kNoTriangle) {
            return triangles[triangle_index].hit(ray, ray_t, record);
        }

        /* ----- Closest Child Hit ----- */
        // The right branch cannot replace a closer left hit because its search
        // interval is capped at the accepted left t.
        ShapeHit left_record;
        const bool hit_left = left->hit(ray, ray_t, left_record, triangles);
        ShapeHit right_record;
        const bool hit_right = right->hit(
            ray, Interval(ray_t.min, hit_left ? left_record.t : ray_t.max),
            right_record, triangles);
        if (hit_right) {
            record = right_record;
        } else if (hit_left) {
            record = left_record;
        }
        return hit_left || hit_right;
    }

    static constexpr std::size_t kNoTriangle =
        static_cast<std::size_t>(-1);
    AABB bounds;
    std::size_t triangle_index = kNoTriangle;
    std::unique_ptr<TriangleBVHNode> left;
    std::unique_ptr<TriangleBVHNode> right;
};

/**
 * @par Implementation
 * Construction validates indexed data, optionally generates missing shading
 * normals, then freezes the buffers shared by all Triangle views. A private
 * index BVH accelerates those views and supplies the aggregate bounds.
 */
Mesh::Mesh(MeshData data, bool generate_normals) {
    /* ----- Validated Attributes ----- */
    validate(data);
    if (generate_normals && data.normals.empty()) {
        generateNormals(data);
    }

    /* ----- Immutable Triangle Views ----- */
    // One shared allocation lets every Triangle view retain only a face index.
    data_ = std::make_shared<const MeshData>(std::move(data));
    triangles_.reserve(data_->triangles.size());
    for (std::size_t index = 0; index < data_->triangles.size(); ++index) {
        triangles_.emplace_back(data_, index);
    }

    /* ----- Private Triangle BVH ----- */
    std::vector<std::size_t> indices(triangles_.size());
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    bvh_ = std::make_unique<TriangleBVHNode>(
        indices, 0, indices.size(), triangles_);
    bounding_box_ = bvh_->bounds;
}

Mesh::~Mesh() = default;
Mesh::Mesh(Mesh&&) noexcept = default;
Mesh& Mesh::operator=(Mesh&&) noexcept = default;

/**
 * @par Implementation
 * Traversal starts at the private local-space BVH. Leaf nodes delegate the
 * geometric solve to `Triangle::hit()`; shrinking child intervals preserves
 * the closest accepted face interaction.
 */
bool Mesh::hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const {
    return bvh_->hit(ray, ray_t, hit, triangles_);
}

/**
 * @par Implementation
 * Validation precedes every indexed access used during Triangle construction.
 * Optional per-vertex arrays must be empty or align with positions; every face
 * must use valid indices and have non-zero cross-product area.
 */
void Mesh::validate(const MeshData& data) {
    /* ----- Required Buffers ----- */
    if (data.positions.empty() || data.triangles.empty()) {
        throw std::invalid_argument("Mesh requires vertices and triangles");
    }

    /* ----- Per-Vertex Alignment ----- */
    if (!data.normals.empty() && data.normals.size() != data.positions.size()) {
        throw std::invalid_argument(
            "Mesh normals must be empty or match the vertex count");
    }
    if (!data.texture_coordinates.empty() &&
        data.texture_coordinates.size() != data.positions.size()) {
        throw std::invalid_argument(
            "Mesh texture coordinates must be empty or match the vertex count");
    }

    /* ----- Indexed Topology ----- */
    for (const auto& triangle : data.triangles) {
        // Validate indices before any indexed position access.
        for (const std::uint32_t index : triangle) {
            if (index >= data.positions.size()) {
                throw std::invalid_argument(
                    "Mesh triangle index is out of range");
            }
        }
        const Vec3f edge_1 = data.positions[triangle[1]] -
                             data.positions[triangle[0]];
        const Vec3f edge_2 = data.positions[triangle[2]] -
                             data.positions[triangle[0]];
        // Zero-area faces have no stable geometric normal or sampling area.
        if (cross(edge_1, edge_2).near_zero()) {
            throw std::invalid_argument(
                "Mesh contains a degenerate triangle");
        }
    }
}

/**
 * @par Implementation
 * Each face contributes `cross(edge1, edge2)` to its three vertices. Its
 * magnitude equals twice the face area, so normalization after accumulation
 * produces area-weighted smooth normals without computing area separately.
 */
void Mesh::generateNormals(MeshData& data) {
    /* ----- Area-Weighted Face Accumulation ----- */
    data.normals.assign(data.positions.size(), Vec3f());
    for (const auto& triangle : data.triangles) {
        const Vec3f edge_1 = data.positions[triangle[1]] -
                             data.positions[triangle[0]];
        const Vec3f edge_2 = data.positions[triangle[2]] -
                             data.positions[triangle[0]];
        const Vec3f area_normal = cross(edge_1, edge_2);
        for (const std::uint32_t index : triangle) {
            data.normals[index] += area_normal;
        }
    }

    /* ----- Vertex Normalization ----- */
    // Unreferenced vertices may retain zero normals; no Triangle can access
    // them, so manufacturing an arbitrary direction would be less correct.
    for (Vec3f& normal : data.normals) {
        if (!normal.near_zero()) normal = unit_vector(normal);
    }
}
