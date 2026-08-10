#include <nearlighter/geometry/sphere.h>

#include <nearlighter/base/onb.h>
#include <nearlighter/math/math.h>
#include <nearlighter/sampling/sampler.h>

Sphere::Sphere()
    : radius(1.0f) {}

Sphere::Sphere(const Point3f& center, const float& radius, std::shared_ptr<Material> material)
    : center(center), radius(std::fmax(0.0f, radius)), material(material),
      moving_center(center, Vec3f(0, 0, 0)) {
    bounding_box = Sphere::calculateAABB(center, radius);
}

Sphere::Sphere(const Point3f& center_start, const Point3f& center_end, const float& radius, std::shared_ptr<Material> material)
    : center(center_start), radius(std::fmax(0.0f, radius)), material(material),
      moving_center(center_start, center_end - center_start) {
    AABB box_start = Sphere::calculateAABB(center_start, radius);
    AABB box_end = Sphere::calculateAABB(center_end, radius);
    bounding_box = AABB(box_start, box_end);
}

/** 
 * Hit a Sphere Primitive
 * 
 * Ray: o + t \vec{d}, Sphere: (p - c)^2 = r^2
 * Hit:
 *  => (o + t \vec{d} - c)^2 = r^2
 *  => \vec{d}^2 t^2 + 2 \vec{d} (o-c) t + (o-c)^2 - r^2 = 0
 *  => Solving the Quadratic equation: a = \vec{d}^2, b = 2\vec{d}(o-c), c = (o-c)^2-r^2
 *  => Solution t should in the ray bound
 */
bool Sphere::hit(const Ray& r, Interval ray_t, HitRecord& hit_record,
                 Sampler&) const {
    return hitDeterministic(r, ray_t, hit_record);
}

bool Sphere::hitDeterministic(const Ray& r, Interval ray_t,
                              HitRecord& hit_record) const {
    Point3f current_center = moving_center.at(r.time());
    Vec3f oc = current_center - r.origin();
    
    float a = dot(r.direction(), r.direction());
    float h = dot(r.direction(), oc);  // h = - b / 2
    float c = dot(oc, oc) - radius * radius;
    float t0, t1;

    if (!solveQuadratic(a, h, c, t0, t1)) return false;
    if (!ray_t.surrounds(t0)) t0 = t1; // if (!r.in_inclusive_bound(t0))
    if (!ray_t.surrounds(t0)) return false;
    
    // than it really hits
    hit_record.t = t0;
    hit_record.point = r.at(t0);
    Vec3f outward_normal = (hit_record.point - current_center) / radius;
    hit_record.set_face_normal(r, outward_normal);
    calculateUV(outward_normal, hit_record.u, hit_record.v);
    hit_record.material = material;

    return true;
}

/**
 * Evaluates uniform solid-angle sampling toward the Sphere.
 *
 * From an external origin, the Sphere occupies a cone with half-angle
 * theta_max. Its visible solid angle is
 *
 *     Omega = 2 pi (1 - cos(theta_max)),
 *     cos(theta_max) = sqrt(1 - radius^2 / distance^2).
 *
 * random() is uniform inside this cone, so every supported direction has
 * density 1 / Omega and every direction missing the Sphere has density zero.
 *
 * @pre origin lies outside the Sphere sampled at motion time zero.
 */
float Sphere::getPDFValue(const Point3f& origin, const Vec3f& direction) const {
    /* ----- Direction support ----- */
    HitRecord record;
    if (!hitDeterministic(Ray(origin, direction),
                          Interval(epsilon, infinity), record)) return 0;

    /* ----- Visible solid angle ----- */
    float distance_squared = (origin - moving_center.at(0)).length_squared();
    float cos_theta_max = std::sqrt(1 - radius * radius / distance_squared);
    float solid_angle = 2 * pi * (1 - cos_theta_max);
    return 1 / solid_angle;
}

/**
 * Samples a direction uniformly over the Sphere's visible solid angle.
 *
 * randomToSphere() samples the view cone around local +z. The ONB then rotates
 * that local direction so +z points from origin to the Sphere center.
 *
 * @pre origin lies outside the Sphere sampled at motion time zero.
 */
Vec3f Sphere::random(const Point3f& origin, Sampler& sampler) const {
    /* ----- View cone ----- */
    Vec3f direction = moving_center.at(0) - origin;  // moving_center.at(0) = center
    float distance_squared = direction.length_squared();

    /* ----- World direction ----- */
    ONB uvw(direction);
    return uvw.transform(randomToSphere(radius, distance_squared, sampler));
}


/* ==================== Private ==================== */

AABB Sphere::calculateAABB(const Point3f& center, const float& radius) {
    Vec3f radius_vec(radius, radius, radius);
    return AABB(center - radius_vec, center + radius_vec);
}

/**
 * Calculate UV coordinates on the sphere
 *  
 * point: point on the unit sphere (radius = 1, center = (0, 0, 0))
 *  point = (x, y, z)
 * u, v: texture coordinates [0, 1]
 * Calculation:
 *     { y = -cos(theta)             { theta = arccos(-y)
 *   - { x = -sin(theta)cos(phi)  => { phi'  = atan2(z,-x) 
 *     { z =  sin(theta)sin(phi)             = phi - pi 
 * 
 *   - { u = phi / 2pi      unification
 *     { v = theta / pi
 */
void Sphere::calculateUV(const Point3f& point, float& u, float& v) {
    float theta = std::acos(-point.y());
    float phi   = std::atan2(-point.z(), point.x()) + pi;
    u = phi / (2 * pi);
    v = theta / pi;
}

/**
 * Samples the +z-aligned view cone by inverse-transform sampling.
 *
 * Uniform solid angle makes cos(theta) uniform on
 * [cos(theta_max), 1] and phi uniform on [0, 2 pi).
 */
Vec3f Sphere::randomToSphere(float radius, float distance_squared,
                             Sampler& sampler) {
    /* ----- Inverse CDF ----- */
    const float r1 = sampler.next1D();
    const float r2 = sampler.next1D();
    float z = 1 + r2 * (std::sqrt(1 - radius * radius / distance_squared) - 1);

    float phi = 2 * pi * r1;
    float sin_theta = std::sqrt(1 - z * z);
    float x = sin_theta * std::cos(phi);
    float y = sin_theta * std::sin(phi);

    return Vec3f(x, y, z);
}
