#include <nearlighter/geometry/triangle.h>

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

AABB calculateBoundingBox(const MeshData& data, std::size_t triangle_index) {
    const auto& indices = data.triangles.at(triangle_index);
    AABB box(data.positions.at(indices[0]), data.positions.at(indices[1]));
    box.uunion(AABB(data.positions.at(indices[2]), data.positions.at(indices[2])));
    return box;
}

}  // namespace


Triangle::Triangle(const Point3f& p0, const Point3f& p1, const Point3f& p2,
                   std::shared_ptr<Material> material)
    : Triangle(makeStandaloneData(p0, p1, p2), 0, std::move(material)) {}

Triangle::Triangle(std::shared_ptr<const MeshData> data,
                   std::size_t triangle_index,
                   std::shared_ptr<Material> material)
    : data_(std::move(data)),
      triangle_index_(triangle_index),
      material_(std::move(material)),
      bounding_box_(calculateBoundingBox(*data_, triangle_index_)) {
    /* ----- Cached geometry ----- */
    const auto& indices = data_->triangles.at(triangle_index_);
    const Vec3f edge_1 = data_->positions.at(indices[1]) -
                         data_->positions.at(indices[0]);
    const Vec3f edge_2 = data_->positions.at(indices[2]) -
                         data_->positions.at(indices[0]);
    const Vec3f area_normal = cross(edge_1, edge_2);
    if (area_normal.near_zero()) {
        throw std::invalid_argument("Triangle vertices must define a non-zero area");
    }
    area_ = 0.5f * area_normal.length();
    geometric_normal_ = unit_vector(area_normal);
}


/**
 * Intersects a ray with a Triangle using the Moller-Trumbore algorithm.
 *
 * Ray:     R(t)      = O  + tD
 * Triangle T(b1, b2) = P0 + b1(P1 - P0) + b2(P2 - P0)
 *
 * Equating both forms produces one linear system for ray parameter t and
 * barycentric coordinates b1 and b2. Cramer's rule reduces that system to
 * dot and cross products without storing a separate plane equation.
 *
 * An intersection is accepted when:
 * - the determinant is non-zero, so the ray is not parallel to the plane;
 * - b1 >= 0, b2 >= 0, and b1 + b2 <= 1;
 * - t lies inside ray_t.
 *
 * The resulting barycentric weights also interpolate optional vertex normals
 * and texture coordinates into the returned HitRecord.
 */
bool Triangle::hit(const Ray& ray, Interval ray_t, HitRecord& hit_record,
                   Sampler&) const {
    return hitDeterministic(ray, ray_t, hit_record);
}

bool Triangle::hitDeterministic(const Ray& ray, Interval ray_t,
                                HitRecord& hit_record) const {
    /* ----- Triangle basis ----- */
    const auto& indices = data_->triangles[triangle_index_];
    const Point3f& p0 = data_->positions[indices[0]];
    const Point3f& p1 = data_->positions[indices[1]];
    const Point3f& p2 = data_->positions[indices[2]];
    const Vec3f edge_1 = p1 - p0;
    const Vec3f edge_2 = p2 - p0;

    /* ----- Moller-Trumbore solve ----- */
    // Solve O + tD = P0 + b1 E1 + b2 E2 without constructing a plane.
    const Vec3f direction_cross_edge_2 = cross(ray.direction(), edge_2);
    const float determinant = dot(edge_1, direction_cross_edge_2);
    // A near-zero determinant means the ray and triangle plane are parallel.
    if (std::fabs(determinant) < kParallelTolerance) return false;

    const float inverse_determinant = 1.0f / determinant;
    const Vec3f origin_offset = ray.origin() - p0;

    // b1 and b2 locate the plane intersection in the triangle edge basis.
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

    /* ----- Interaction attributes ----- */
    const float barycentric_0 = 1.0f - barycentric_1 - barycentric_2;
    hit_record.t = t;
    hit_record.point = ray.at(t);

    // Geometric orientation defines front_face even when shading is smooth.
    hit_record.set_face_normal(ray, geometric_normal_);

    /* ----- Shading normal ----- */
    if (data_->normals.size() == data_->positions.size()) {
        // Barycentric interpolation makes adjacent faces appear continuous.
        Vec3f shading_normal =
            barycentric_0 * data_->normals[indices[0]] +
            barycentric_1 * data_->normals[indices[1]] +
            barycentric_2 * data_->normals[indices[2]];
        if (!shading_normal.near_zero()) {
            shading_normal = unit_vector(shading_normal);
            // Keep imported normals in the geometric normal's hemisphere.
            if (dot(shading_normal, geometric_normal_) < 0.0f) {
                shading_normal = -shading_normal;
            }
            hit_record.normal = hit_record.front_face
                                    ? shading_normal
                                    : -shading_normal;
        }
    }

    /* ----- Texture coordinates ----- */
    if (data_->texture_coordinates.size() == data_->positions.size()) {
        const auto& uv0 = data_->texture_coordinates[indices[0]];
        const auto& uv1 = data_->texture_coordinates[indices[1]];
        const auto& uv2 = data_->texture_coordinates[indices[2]];
        hit_record.u = barycentric_0 * uv0[0] + barycentric_1 * uv1[0] +
                       barycentric_2 * uv2[0];
        hit_record.v = barycentric_0 * uv0[1] + barycentric_1 * uv1[1] +
                       barycentric_2 * uv2[1];
    } else {
        // Barycentric coordinates provide a stable local fallback mapping.
        hit_record.u = barycentric_1;
        hit_record.v = barycentric_2;
    }
    hit_record.material = material_;
    return true;
}

// ==================================================
// Area Sampling
// ==================================================

float Triangle::getPDFValue(const Point3f& origin,
                            const Vec3f& direction) const {
    /* ----- Visibility domain ----- */
    // Directions outside the triangle have zero density for this strategy.
    HitRecord record;
    if (!hitDeterministic(
            Ray(origin, direction), Interval(epsilon, infinity), record)) {
        return 0.0f;
    }

    /* ----- Area-to-solid-angle conversion ----- */
    // Uniform p_A = 1 / area becomes p_omega = distance^2 / (cosine * area).
    const float distance_squared =
        record.t * record.t * direction.length_squared();
    const float cosine = std::fabs(
        dot(unit_vector(direction), geometric_normal_));
    if (cosine <= 0.0f) return 0.0f;
    return distance_squared / (cosine * area_);
}

Vec3f Triangle::random(const Point3f& origin, Sampler& sampler) const {
    /* ----- Surface sample ----- */
    const auto& indices = data_->triangles[triangle_index_];
    const Point3f& p0 = data_->positions[indices[0]];
    const Point3f& p1 = data_->positions[indices[1]];
    const Point3f& p2 = data_->positions[indices[2]];

    // Square-root warping produces uniform density over triangle area.
    const float root = std::sqrt(sampler.next1D());
    const float barycentric_0 = 1.0f - root;
    const float barycentric_1 = root * (1.0f - sampler.next1D());
    const float barycentric_2 = 1.0f - barycentric_0 - barycentric_1;
    const Point3f point = barycentric_0 * p0 + barycentric_1 * p1 +
                          barycentric_2 * p2;

    // ShapePDF expects a world-space direction, not the sampled position.
    return point - origin;
}
