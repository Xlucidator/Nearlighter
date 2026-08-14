#ifndef NEARLIGHTER_SHAPE_SPHERE_H
#define NEARLIGHTER_SHAPE_SPHERE_H

#include <nearlighter/shape/shape.h>

/**
 * Sphere
 *
 * Represents a positive-radius local sphere. Its center is either stationary
 * or moves linearly as a function of ray time over `[0,1]`.
 */
class Sphere final : public Shape {
public:
    /**
     * @name Sphere Construction
     * Creates a stationary or linearly moving sphere with positive radius.
     *
     * @throws std::invalid_argument if `radius` is not positive.
     * @{
     */
    Sphere(const Point3f& center, float radius);
    Sphere(const Point3f& center_start, const Point3f& center_end,
           float radius);
    /** @} */

    bool hit(const Ray& ray, Interval ray_t, ShapeHit& hit) const override;

    const AABB& getBoundingBox() const override { return bounding_box_; }

    bool hasPDF() const override { return true; }

    /**
     * @name Uniform Solid-Angle Sampling
     * Uses the center at motion time zero.
     *
     * - Outside origins: uniform directions over the visible cone.
     * - Inside origins: uniform directions over the full unit sphere.
     * @{
     */
    float getPDFValue(const Point3f& origin,
                      const Vec3f& direction) const override;
    Vec3f random(const Point3f& origin, Sampler& sampler) const override;
    /** @} */

private:
    static AABB calculateAABB(const Point3f& center, float radius);
    static void calculateUV(const Point3f& point, float& u, float& v);
    static Vec3f randomToSphere(float radius, float distance_squared,
                                Sampler& sampler);

    Point3f center_;
    float radius_;
    Ray moving_center_;
    AABB bounding_box_;
};

#endif  // NEARLIGHTER_SHAPE_SPHERE_H
