#include "camera.h"


/////// Private Method

void Camera::initialize() {
    /* Image */
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    /* Sampling */
    pixel_samples_scale = 1.0f / samples_per_pixel;

    /* Viewport */
    center = Point3f(0.0f, 0.0f, 0.0f);
    float viewport_height = 2.0f;
    float viewport_width = viewport_height * (float(image_width) / image_height);
    float focal_length = 1.0f;
    
    Vec3f viewport_u = Vec3f(viewport_width, 0.0f, 0.0f);     // vector for u-axis: point rightward
    Vec3f viewport_v = Vec3f(0.0f, -viewport_height, 0.0f);   // vector for v-axis: point downward
    pixel_interval_u = viewport_u / image_width;   // vector indicates pixel interval of u direction
    pixel_interval_v = viewport_v / image_height;  // vector indicates pixel interval of v direction

    Point3f viewport_upper_left = center - Vec3f(0.0f, 0.0f, focal_length)
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

Color Camera::getRayColor(const Ray& r, const Shape& world) const {
    /* Objects */
    HitRecord record;
    if (world.hit(r, Interval(0.0f, infinity), record)) {
        return 0.5f * (record.normal + Color(1.0f, 1.0f, 1.0f));
    }

    /* Background */
    Vec3f unit_direction = unit_vector(r.direction());
    auto a = 0.5f * (unit_direction.y() + 1.0f);
    return (1.0f-a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
}


/////// Public Method

void Camera::render(const Shape& world) {
    initialize();

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanling remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            /* Generate Rays and Get Color */
            Color pixel_color(0.0f, 0.0f, 0.0f);
            for (int sample = 0; sample < samples_per_pixel; sample++) {
                pixel_color += getRayColor(getRay(i, j), world);
            }
            pixel_color *= pixel_samples_scale;

            /* Draw */
            write_color(std::cout, pixel_color);
        }
    }
    std::clog << "\rDone.                 \n";
}