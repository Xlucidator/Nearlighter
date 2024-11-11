#include "camera.h"


/////// Private Method

void Camera::initialize() {
    /* Image */
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    /* Viewport */
    center = Point3f(0, 0, 0);
    float viewport_height = 2.0;
    float viewport_width = viewport_height * (float(image_width) / image_height);
    float focal_length = 1.0;
    
    Vec3f viewport_u = Vec3f(viewport_width, 0, 0);     // vector for u-axis: point rightward
    Vec3f viewport_v = Vec3f(0, -viewport_height, 0);   // vector for v-axis: point downward
    pixel_interval_u = viewport_u / image_width;   // vector indicates pixel interval of u direction
    pixel_interval_v = viewport_v / image_height;  // vector indicates pixel interval of v direction

    Point3f viewport_upper_left = center - Vec3f(0, 0, focal_length)
                                - viewport_u / 2 - viewport_v / 2;  // notice that v has reversed
    pixel_start = viewport_upper_left + 0.5 * (pixel_interval_u + pixel_interval_v); // pixels should locate in the grid center 
}


Color Camera::ray_color(const Ray& r, const Shape& world) const {
    /* Objects */
    HitRecord record;
    if (world.hit(r, record)) {
        return 0.5 * (record.normal + Color(1, 1, 1));
    }

    /* Background */
    Vec3f unit_direction = unit_vector(r.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0-a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
}


/////// Public Method

void Camera::render(const Shape& world) {
    initialize();

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanling remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            /* Generate a ray */
            auto pixel_center = pixel_start + (i * pixel_interval_u) + (j * pixel_interval_v); // not elegant, can be accelerate
            Vec3f ray_direction = pixel_center - center;
            Ray ray(center, ray_direction);

            /* Get ray color and draw to the canvas */
            Color pixel_color = ray_color(ray, world);
            write_color(std::cout, pixel_color);
        }
    }
    std::clog << "\rDone.                 \n";
}