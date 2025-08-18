#include "camera.h"
#include "utils/pdf.h"
#include "material/material.h"

/////// Public Method

void Camera::render(const Shape& world, const Shape& lights, std::ostream& out) {
    initialize();

    out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanling remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            /* Generate Rays and Get Color */
            Color pixel_color(0.0f, 0.0f, 0.0f);
            // Stratified Sampling
            for (int sample_i = 0; sample_i < sqrt_spp; sample_i++) {
                for (int sample_j = 0; sample_j < sqrt_spp; sample_j++) {
                    pixel_color += getRayColor(getRay(i, j, sample_i, sample_j), max_depth, world, lights);
                }
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
    sqrt_spp = int(std::sqrt(samples_per_pixel));
    pixel_samples_scale = 1.0f / (sqrt_spp * sqrt_spp);
    reverse_sqrt_spp = 1.0 / sqrt_spp;

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

/* Get ray for sample pixel around the origin pixel point 
 *  - sample point: located around (pixel_x, pixel_y)
 *  - straitified sampling: the target pixel was divided into grid of sqrt_spp * sqrt_spp
 *    => offset in the pixel: (sample_x, sample_y)
 */
Ray Camera::getRay(int pixel_x, int pixel_y, int sample_x, int sample_y) const {
    Vec3f offset = genSampleSquareStratified(sample_x, sample_y); // genSampleSquare();
    Vec3f pixel_sample = pixel_start 
                    + (pixel_x + offset.x()) * pixel_interval_u
                    + (pixel_y + offset.y()) * pixel_interval_v;

    Vec3f ray_origin = (defocus_angle <= 0) ? center : genSampleDeforceDisk();
    float ray_time = random_float();

    return Ray(ray_origin, pixel_sample - ray_origin, ray_time); // TODO: is unit_vector(pixel_sample - ray_origin) needed (or better needed) ?
}

Vec3f Camera::genSampleSquare() const {
    return Vec3f(random_float() - 0.5f, random_float() - 0.5f, 0);
}

// Stratified: (sx, sy) in 'sqrt_spp * sqrt_spp' grid of [0, 1]
Vec3f Camera::genSampleSquareStratified(int sx, int sy) const {
    float px = (sx + random_float()) * reverse_sqrt_spp;
    float py = (sy + random_float()) * reverse_sqrt_spp;
    return Vec3f(px - 0.5f, py - 0.5f, 0);
}

Vec3f Camera::genSampleDeforceDisk() const {
    Vec3f p = random_unit_in_disk();
    return center + p.x() * defocus_disk_u + p.y() * defocus_disk_v;
}

Color Camera::getRayColor(const Ray& ray, int depth, const Shape& world, const Shape& lights) const {
    if (depth <= 0) return Color(0.0f, 0.0f, 0.0f);

    HitRecord record;
    /* Background */
    if (!world.hit(ray, Interval(0.001f, infinity), record)) return background;

    /* Objects */
    Ray scattered;
    Color attenuation;
    float pdf_value;
    Color color_from_emitted = record.material->emitted(ray, record, record.u, record.v, record.point);
    
    // check and get scattered ray
    if (!record.material->scatter(ray, record, attenuation, scattered, pdf_value)) 
        return color_from_emitted;
    
    // sample to light
    ShapePDF light_pdf(lights, record.point);
    scattered = Ray(record.point, light_pdf.generate(), ray.time());
    pdf_value = light_pdf.value(scattered.direction());

    // // sample to surface
    // CosineHemispherePDF surface_pdf(record.normal);
    // scattered = Ray(record.point, surface_pdf.generate(), ray.time());
    // pdf_value = surface_pdf.value(scattered.direction());
    
    // brdf - scatter pdf
    float scattering_pdf = record.material->getScatterPDF(ray, record, scattered);

    Color sample_color = getRayColor(scattered, depth-1, world, lights);
    Color color_from_scatter = attenuation * scattering_pdf * sample_color / pdf_value;
    
    return color_from_emitted + color_from_scatter;
}
