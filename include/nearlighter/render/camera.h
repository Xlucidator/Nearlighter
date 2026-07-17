#ifndef NEARLIGHTER_RENDER_CAMERA_H
#define NEARLIGHTER_RENDER_CAMERA_H

#include <nearlighter/base/ray.h>

class Sampler;

/**
 * Stores the physical and projective parameters of a perspective camera.
 *
 * Resolution and sampling quality are render settings rather than camera
 * properties. Call prepare() once for a render to derive resolution-dependent
 * ray-generation state.
 */
class Camera {
public:
    // Perspective projection and world-space pose.
    float vertical_fov = 90.0f;
    Point3f position = Point3f(0.0f, 0.0f, 0.0f);
    Point3f look_at = Point3f(0.0f, 0.0f, -1.0f);
    Vec3f world_up = Vec3f(0.0f, 1.0f, 0.0f);

    // Thin-lens parameters; zero defocus disables depth of field.
    float defocus_angle = 0.0f;
    float focus_distance = 10.0f;

    class Prepared;

    /**
     * Precomputes the camera frame and pixel geometry for one output size.
     *
     * @throws std::invalid_argument if the resolution or camera configuration
     * is degenerate.
     */
    Prepared prepare(int image_width, int image_height) const;
};

/** Holds immutable, resolution-dependent primary-ray generation state. */
class Camera::Prepared {
public:
    /** Generates one jittered primary ray for the requested pixel. */
    Ray generateRay(int pixel_x, int pixel_y, Sampler& sampler) const;

private:
    friend class Camera;

    Prepared(int image_width, int image_height, const Point3f& center,
             const Point3f& pixel_start, const Vec3f& pixel_interval_u,
             const Vec3f& pixel_interval_v, const Vec3f& defocus_disk_u,
             const Vec3f& defocus_disk_v, bool use_defocus);

    int image_width_;
    int image_height_;
    Point3f center_;
    Point3f pixel_start_;
    Vec3f pixel_interval_u_;
    Vec3f pixel_interval_v_;
    Vec3f defocus_disk_u_;
    Vec3f defocus_disk_v_;
    bool use_defocus_;
};

#endif  // NEARLIGHTER_RENDER_CAMERA_H
