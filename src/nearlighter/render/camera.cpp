#include <nearlighter/render/camera.h>

#include <nearlighter/math/math.h>
#include <nearlighter/sampling/sampler.h>

#include <cmath>
#include <stdexcept>

Camera::Prepared Camera::prepare(int image_width, int image_height) const {
    if (image_width <= 0 || image_height <= 0) {
        throw std::invalid_argument("Camera output dimensions must be positive");
    }
    if (vertical_fov <= 0.0f || vertical_fov >= 180.0f) {
        throw std::invalid_argument("Camera vertical field of view must be in (0, 180)");
    }
    if (focus_distance <= 0.0f) {
        throw std::invalid_argument("Camera focus distance must be positive");
    }
    if (defocus_angle < 0.0f || defocus_angle >= 180.0f) {
        throw std::invalid_argument("Camera defocus angle must be in [0, 180)");
    }

    /* Establish an orthonormal camera basis in world space. */
    const Vec3f view_direction = look_at - position;
    if (view_direction.near_zero()) {
        throw std::invalid_argument("Camera position and look-at point must differ");
    }
    const Vec3f front = unit_vector(view_direction);
    const Vec3f right_unnormalized = cross(front, world_up);
    if (right_unnormalized.near_zero()) {
        throw std::invalid_argument("Camera world-up vector must not be parallel to its view direction");
    }
    const Vec3f right = unit_vector(right_unnormalized);
    const Vec3f up = cross(right, front);

    /* Project the requested image aspect ratio onto the focus plane. */
    const float half_height =
        std::tan(degrees_to_radians(vertical_fov) * 0.5f) * focus_distance;
    const float viewport_height = 2.0f * half_height;
    const float aspect_ratio = static_cast<float>(image_width) /
                               static_cast<float>(image_height);
    const float viewport_width = viewport_height * aspect_ratio;
    const Vec3f viewport_u = viewport_width * right;
    const Vec3f viewport_v = viewport_height * -up;
    const Vec3f pixel_interval_u = viewport_u / static_cast<float>(image_width);
    const Vec3f pixel_interval_v = viewport_v / static_cast<float>(image_height);
    const Point3f viewport_upper_left =
        position + focus_distance * front - viewport_u * 0.5f -
        viewport_v * 0.5f;
    const Point3f pixel_start =
        viewport_upper_left + 0.5f * (pixel_interval_u + pixel_interval_v);

    const float defocus_radius =
        focus_distance * std::tan(degrees_to_radians(defocus_angle) * 0.5f);
    return Prepared(image_width, image_height, position, pixel_start,
                    pixel_interval_u, pixel_interval_v,
                    defocus_radius * right, defocus_radius * up,
                    defocus_angle > 0.0f);
}

Camera::Prepared::Prepared(int image_width, int image_height,
                           const Point3f& center,
                           const Point3f& pixel_start,
                           const Vec3f& pixel_interval_u,
                           const Vec3f& pixel_interval_v,
                           const Vec3f& defocus_disk_u,
                           const Vec3f& defocus_disk_v,
                           bool use_defocus)
    : image_width_(image_width),
      image_height_(image_height),
      center_(center),
      pixel_start_(pixel_start),
      pixel_interval_u_(pixel_interval_u),
      pixel_interval_v_(pixel_interval_v),
      defocus_disk_u_(defocus_disk_u),
      defocus_disk_v_(defocus_disk_v),
      use_defocus_(use_defocus) {}

Ray Camera::Prepared::generateRay(int pixel_x, int pixel_y,
                                  Sampler& sampler) const {
    if (pixel_x < 0 || pixel_x >= image_width_ ||
        pixel_y < 0 || pixel_y >= image_height_) {
        throw std::out_of_range("Primary-ray pixel is outside the prepared camera");
    }

    // Center the uniform subpixel offset around the addressed pixel center.
    const float offset_x = sampler.next1D() - 0.5f;
    const float offset_y = sampler.next1D() - 0.5f;
    const Point3f pixel_sample =
        pixel_start_ + (static_cast<float>(pixel_x) + offset_x) *
                           pixel_interval_u_ +
        (static_cast<float>(pixel_y) + offset_y) * pixel_interval_v_;

    Point3f ray_origin = center_;
    if (use_defocus_) {
        const Vec3f disk_sample = sampler.nextInUnitDisk();
        ray_origin += disk_sample.x() * defocus_disk_u_ +
                      disk_sample.y() * defocus_disk_v_;
    }

    return Ray(ray_origin, pixel_sample - ray_origin, sampler.next1D());
}
