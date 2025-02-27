#include "camera.h"
#include "material/material.h"

/////// Public Method

void Camera::render(const Shape& world, std::ostream& out) {
    initialize();

    out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanling remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            /* Generate Rays and Get Color */
            Color pixel_color(0.0f, 0.0f, 0.0f);
            for (int sample = 0; sample < samples_per_pixel; sample++) {
                pixel_color += getRayColor(getRay(i, j), max_depth, world);
            }
            pixel_color *= pixel_samples_scale;

            /* Draw */
            writeColor(out, pixel_color);
        }
    }
    std::clog << "\rDone.                 \n";
}


/////// Private Method

void Camera::initialize() {
    /* Image */
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    /* Sampling */
    pixel_samples_scale = 1.0f / samples_per_pixel;

    /* Viewport */
    // Calculate Viewport Size
    //         ·
    //    y   /| 
    //    ^  / | h = tan(fov_angle/2)
    //    | /  |         
    //    0----1--------(|)----> -z
    //              focal_length = focus_distance, place viewport here
    center = position;
    float h = std::tan(degrees_to_radians(fov_vertical) / 2);
    float viewport_height = 2 * h * focus_distance;
    float viewport_width = viewport_height * (float(image_width) / image_height);

    // Calculate Camera Pose (Should avoid illegal case)
    front = unit_vector(look_at - position);
    right = unit_vector(cross(front, world_up));
    up = cross(right, front);
    
    // Calculate Viewport in 3D space
    Vec3f viewport_u = viewport_width * right;  // vector for u-axis: point rightward
    Vec3f viewport_v = viewport_height * -up ;  // vector for v-axis: point downward
    pixel_interval_u = viewport_u / image_width;   // vector indicates pixel interval of u direction
    pixel_interval_v = viewport_v / image_height;  // vector indicates pixel interval of v direction

    Point3f viewport_upper_left = center + focus_distance * front
                                - viewport_u / 2 - viewport_v / 2;  // notice that v has reversed
    pixel_start = viewport_upper_left + 0.5f * (pixel_interval_u + pixel_interval_v); // pixels should locate in the grid center 

    // Calculate camera Defocus Disk Basis Vectors
    float defocus_radius = focus_distance * std::tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = defocus_radius * right;
    defocus_disk_v = defocus_radius * up;
}

// Get ray for sample pixel around the origin pixel point 
Ray Camera::getRay(int pixel_x, int pixel_y) const {
    Vec3f offset = genSampleSquare();
    Vec3f pixel_sample = pixel_start 
                    + (pixel_x + offset.x()) * pixel_interval_u
                    + (pixel_y + offset.y()) * pixel_interval_v;

    Vec3f ray_origin = (defocus_angle <= 0) ? center : genSampleDeforceDisk();
    float ray_time = random_float();

    return Ray(ray_origin, pixel_sample - ray_origin, ray_time);
}

Vec3f Camera::genSampleSquare() const {
    return Vec3f(random_float() - 0.5f, random_float() - 0.5f, 0);
}

Vec3f Camera::genSampleDeforceDisk() const {
    Vec3f p = random_unit_in_disk();
    return center + p.x() * defocus_disk_u + p.y() * defocus_disk_v;
}

Color Camera::getRayColor(const Ray& ray, int depth, const Shape& world) const {
    if (depth <= 0) return Color(0.0f, 0.0f, 0.0f);

    HitRecord record;
    /* Background */
    if (!world.hit(ray, Interval(0.001f, infinity), record)) return background;

    /* Objects */
    Ray scattered;
    Color attenuation;

    Color color_from_emitted = record.material->emitted(record.u, record.v, record.point);
    if (!record.material->scatter(ray, record, attenuation, scattered)) return color_from_emitted;
    Color color_from_scatter = attenuation * getRayColor(scattered, depth-1, world);

    return color_from_emitted + color_from_scatter;
}
