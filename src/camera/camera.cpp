#include "camera.h"


/////// Private Method

void Camera::initialize() {
    /* Image */
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    /* Sampling */
    pixel_samples_scale = 1.0f / samples_per_pixel;

    /* Viewport */
    //         ·
    //    y   /|
    //    ^  / | h = tan(fov_angle/2)
    //    | /  | 
    //    0----1-----·-> -z
    //          focal_length 
    center = position;
    float focal_length = (position - look_at).length();
    float h = std::tan(degrees_to_radians(fov_vertical) / 2);
    float viewport_height = 2 * h * focal_length;
    float viewport_width = viewport_height * (float(image_width) / image_height);

    // Calculate Camera Pose : avoid illegal case
    front = unit_vector(look_at - position);
    right = unit_vector(cross(front, world_up));
    up = cross(right, front);
    
    Vec3f viewport_u = viewport_width * right;  // vector for u-axis: point rightward
    Vec3f viewport_v = viewport_height * -up ;  // vector for v-axis: point downward
    pixel_interval_u = viewport_u / image_width;   // vector indicates pixel interval of u direction
    pixel_interval_v = viewport_v / image_height;  // vector indicates pixel interval of v direction

    Point3f viewport_upper_left = center + focal_length * front
                                - viewport_u / 2 - viewport_v / 2;  // notice that v has reversed
    pixel_start = viewport_upper_left + 0.5f * (pixel_interval_u + pixel_interval_v); // pixels should locate in the grid center 
}

// Get ray for sample pixel around the origin pixel point 
Ray Camera::getRay(int pixel_x, int pixel_y) const {
    Vec3f offset = genSampleSquare();
    Vec3f pixel_sample = pixel_start 
                    + (pixel_x + offset.x()) * pixel_interval_u
                    + (pixel_y + offset.y()) * pixel_interval_v;

    return Ray(center, pixel_sample - center);
}

Vec3f Camera::genSampleSquare() const {
    return Vec3f(random_float() - 0.5f, random_float() - 0.5f, 0);
}

Color Camera::getRayColor(const Ray& ray, int depth, const Shape& world) const {
    if (depth <= 0) return Color(0.0f, 0.0f, 0.0f);

    /* Objects */
    HitRecord record;
    if (world.hit(ray, Interval(0.001f, infinity), record)) {
        Ray scattered; // Deal with material here
        Color attenuation;
        if (record.material->scatter(ray, record, attenuation, scattered))
            return attenuation * getRayColor(scattered, depth-1, world);
        return Color(0.0f, 0.0f, 0.0f);
    }

    /* Background */
    Vec3f unit_direction = unit_vector(ray.direction());
    auto a = 0.5f * (unit_direction.y() + 1.0f);
    return (1.0f-a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
}


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