#ifndef ROTATE_H
#define ROTATE_H

#include "shape/shape.h"

#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>

inline glm::vec3 to_glm_vec3(const Vec3f& v) { return glm::vec3(v.x(), v.y(), v.z()); }
inline Vec3f to_vec3f(const glm::vec3& v) { return Vec3f(v.x, v.y, v.z); }  // glm::vec3 -> Vec3f
inline Point3f to_point3f(const glm::vec4& v) { return Point3f(v.x, v.y, v.z); }  // glm::vec4 -> Point3f == Vec3f

class Rotate : public Shape {
public:
    Rotate(shared_ptr<Shape> shape, const Vec3f& axis, float angle) : shape(shape) {
        rotation = glm::rotate(glm::mat4(1.0f), angle, to_glm_vec3(axis));
        inv_rotation = glm::inverse(rotation);
        calculateRotatedBoundingBox(shape->getBoundingBox(), rotation);
    }
    Rotate(shared_ptr<Shape> shape, glm::mat4 rotation) : shape(shape), rotation(rotation) {
        inv_rotation = glm::inverse(rotation);
        calculateRotatedBoundingBox(shape->getBoundingBox(), rotation);
    }

    bool hit(const Ray& ray, Interval ray_t, HitRecord& record) const override {
        /* Transform the ray from world space to object space */
        Vec3f rotated_origin = to_vec3f(inv_rotation * glm::vec4(to_glm_vec3(ray.origin()), 1.0f));
        Vec3f rotated_direction = to_vec3f(inv_rotation * glm::vec4(to_glm_vec3(ray.direction()), 0.0f));
        Ray rotated_ray = Ray(rotated_origin, rotated_direction);

        // Hit object with rotated ray
        if (!shape->hit(rotated_ray, ray_t, record))
            return false;

        /* Transform the intersection from object space back to world space */
        record.point = to_point3f(rotation * glm::vec4(to_glm_vec3(record.point), 1.0f));
        record.normal = to_point3f(rotation * glm::vec4(to_glm_vec3(record.normal), 0.0f));
        return true;
    }

    const AABB& getBoundingBox() const override { return bbox; }

private:
    shared_ptr<Shape> shape;
    glm::mat4 rotation;
    glm::mat4 inv_rotation;
    AABB bbox;

    void calculateRotatedBoundingBox(const AABB& box, const glm::mat4& rotate) { // TODO: better implement in AABB class
        Point3f p_min( infinity,  infinity,  infinity);
        Point3f p_max(-infinity, -infinity, -infinity);
        for (int i = 0; i < 8; i++) {
            Point3f p = box.corner(i);
            Point3f rotated_p = to_point3f(rotate * glm::vec4(p.x(), p.y(), p.z(), 1.0f));
            for (int ax = 0; ax < 3; ax++) {
                p_min[ax] = std::fmin(p_min[ax], rotated_p[ax]);
                p_max[ax] = std::fmax(p_max[ax], rotated_p[ax]);
            }
        }
        bbox = AABB(p_min, p_max);
    }
};


#endif // ROTATE_H
