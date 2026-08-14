#include <nearlighter/shape/triangle.h>

#include <nearlighter/shape/mesh.h>

#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace {

constexpr float kParallelTolerance = 1e-8f;

std::shared_ptr<const MeshData> makeStandaloneData(
    const Point3f& p0, const Point3f& p1, const Point3f& p2) {
    auto data = std::make_shared<MeshData>();
    data->positions = {p0, p1, p2};
    data->triangles.push_back({0, 1, 2});
    return data;
}

AABB calculateBoundingBox(const MeshData& data,
                          std::size_t triangle_index) {
    const auto& indices = data.triangles.at(triangle_index);
    AABB bounds(data.positions.at(indices[0]), data.positions.at(indices[1]));
    bounds.uunion(AABB(data.positions.at(indices[2]),
                      data.positions.at(indices[2])));
    return bounds;
}

}  // namespace

Triangle::Triangle(const Point3f& p0, const Point3f& p1,
                   const Point3f& p2)
    : Triangle(makeStandaloneData(p0, p1, p2), 0) {}

Triangle::Triangle(std::shared_ptr<const MeshData> data,
                   std::size_t triangle_index)
    : data_(std::move(data)),
      triangle_index_(triangle_index) {
    if (!data_) {
        throw std::invalid_argument("Triangle requires MeshData");
    }
    bounding_box_ = calculateBoundingBox(*data_, triangle_index_);

    const auto& indices = data_->triangles.at(triangle_index_);
    const Vec3f edge_1 = data_->positions.at(indices[1]) -
                         data_->positions.at(indices[0]);
    const Vec3f edge_2 = data_->positions.at(indices[2]) -
                         data_->positions.at(indices[0]);
    const Vec3f area_normal = cross(edge_1, edge_2);
    if (area_normal.near_zero()) {
        throw std::invalid_argument(
            "Triangle vertices must define a non-zero area");
    }
    area_ = 0.5f * area_normal.length();
    geometric_normal_ = unit_vector(area_normal);
}

/**
 * @par Implementation
 * Moller-Trumbore solves the equality
 *
 *     O + tD = P0 + b1*(P1 - P0) + b2*(P2 - P0)
 *
 * Let `E1 = P1 - P0`, `E2 = P2 - P0`, `S = O - P0`,
 * `P = D cross E2`, and `Q = S cross E1`. Cramer's rule gives
 *
 *     determinant = E1 dot P
 *     b1 = (S dot P) / determinant
 *     b2 = (D dot Q) / determinant
 *     t  = (E2 dot Q) / determinant.
 *
 * A hit requires:
 * - non-zero determinant: the ray is not parallel to the triangle plane;
 * - `b1 >= 0`, `b2 >= 0`, and `b1 + b2 <= 1`;
 * - `t` inside the accepted ray interval.
 *
 * The remaining weight `b0 = 1 - b1 - b2` interpolates optional normals and
 * texture coordinates after the geometric hit is accepted.
 */
bool Triangle::hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const {
    /* ----- Indexed Triangle Geometry ----- */
    const auto& indices = data_->triangles[triangle_index_];
    const Point3f& p0 = data_->positions[indices[0]];
    const Point3f& p1 = data_->positions[indices[1]];
    const Point3f& p2 = data_->positions[indices[2]];
    const Vec3f edge_1 = p1 - p0;
    const Vec3f edge_2 = p2 - p0;

    /* ----- Moller-Trumbore Coordinates ----- */
    const Vec3f direction_cross_edge_2 = cross(ray.direction(), edge_2);
    const float determinant = dot(edge_1, direction_cross_edge_2);
    if (std::fabs(determinant) < kParallelTolerance) return false;

    const float inverse_determinant = 1.0f / determinant;
    const Vec3f origin_offset = ray.origin() - p0;
    const float barycentric_1 =
        dot(origin_offset, direction_cross_edge_2) * inverse_determinant;
    if (barycentric_1 < 0.0f || barycentric_1 > 1.0f) return false;

    const Vec3f origin_cross_edge_1 = cross(origin_offset, edge_1);
    const float barycentric_2 =
        dot(ray.direction(), origin_cross_edge_1) * inverse_determinant;
    if (barycentric_2 < 0.0f ||
        barycentric_1 + barycentric_2 > 1.0f) {
        return false;
    }

    const float t = dot(edge_2, origin_cross_edge_1) * inverse_determinant;
    if (!ray_t.surrounds(t)) return false;

    /* ----- Geometric Interaction ----- */
    const float barycentric_0 = 1.0f - barycentric_1 - barycentric_2;
    hit.t = t;
    hit.point = ray.at(t);
    hit.geometric_normal = geometric_normal_;
    hit.shading_normal = geometric_normal_;

    /* ----- Optional Interpolated Attributes ----- */
    if (data_->normals.size() == data_->positions.size()) {
        Vec3f shading_normal =
            barycentric_0 * data_->normals[indices[0]] +
            barycentric_1 * data_->normals[indices[1]] +
            barycentric_2 * data_->normals[indices[2]];
        if (!shading_normal.near_zero()) {
            shading_normal = unit_vector(shading_normal);
            if (dot(shading_normal, geometric_normal_) < 0.0f) {
                shading_normal = -shading_normal;
            }
            hit.shading_normal = shading_normal;
        }
    }

    if (data_->texture_coordinates.size() == data_->positions.size()) {
        const auto& uv0 = data_->texture_coordinates[indices[0]];
        const auto& uv1 = data_->texture_coordinates[indices[1]];
        const auto& uv2 = data_->texture_coordinates[indices[2]];
        hit.u = barycentric_0 * uv0[0] + barycentric_1 * uv1[0] +
                barycentric_2 * uv2[0];
        hit.v = barycentric_0 * uv0[1] + barycentric_1 * uv1[1] +
                barycentric_2 * uv2[1];
    } else {
        hit.u = barycentric_1;
        hit.v = barycentric_2;
    }
    return true;
}

/**
 * @par Implementation
 * Uniform area density converts to solid angle by
 *
 *     p_omega = distance^2 / (abs(N dot omega) * area).
 */
float Triangle::getPDFValue(const Point3f& origin,
                            const Vec3f& direction) const {
    ShapeHit hit_record;
    if (!hit(Ray(origin, direction), Interval(epsilon, infinity),
             hit_record)) {
        return 0.0f;
    }
    const float distance_squared =
        hit_record.t * hit_record.t * direction.length_squared();
    const float cosine = std::fabs(
        dot(unit_vector(direction), geometric_normal_));
    if (cosine <= 0.0f) return 0.0f;
    return distance_squared / (cosine * area_);
}

Vec3f Triangle::random(const Point3f& origin, Sampler& sampler) const {
    const auto& indices = data_->triangles[triangle_index_];
    const Point3f& p0 = data_->positions[indices[0]];
    const Point3f& p1 = data_->positions[indices[1]];
    const Point3f& p2 = data_->positions[indices[2]];
    // Square-root warping gives uniform density over triangle area.
    const float root = std::sqrt(sampler.next1D());
    const float barycentric_0 = 1.0f - root;
    const float barycentric_1 = root * (1.0f - sampler.next1D());
    const float barycentric_2 = 1.0f - barycentric_0 - barycentric_1;
    return barycentric_0 * p0 + barycentric_1 * p1 +
           barycentric_2 * p2 - origin;
}
