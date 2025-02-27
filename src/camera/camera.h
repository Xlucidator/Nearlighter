#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"
#include "shape/shape.h"

class Camera {
public:
    float	aspect_ratio = 1.0f;// Ratio of image width over height
    int 	image_width = 100;	// Rendered image width
    int     samples_per_pixel = 10; // Render samples for each pixel
    int     max_depth = 10;     // Max bounce times for a ray
    Color   background = Color(0.70f, 0.80f, 1.00f);

    float   fov_vertical = 90.0f;   // Vertical view angle
    Point3f position = Point3f(0.0f, 0.0f,  0.0f);
    Point3f look_at  = Point3f(0.0f, 0.0f, -1.0f);
    Vec3f   world_up = Vec3f(0.0f, 1.0f, 0.0f);

    float   defocus_angle = 0.0f;   // Variation angle of ray through each pixel
    float   focus_distance = 10.0f; // Camera focus distance: camera position <--> plane of perfect focus

    void render(const Shape& world, std::ostream& out = std::cout);

private:
    int     image_height;		// Rendered image height
    float   pixel_samples_scale;// Color scale factor
    Point3f center;				// Camera center location in world coordinate
    Point3f pixel_start;		// Location of pixel[0][0] in world coordinate
    Vec3f   pixel_interval_u;	// Intervals between pixel in u direction in world coordinate
    Vec3f   pixel_interval_v;	// Intervals between pixel in v direction in world coordinate

    Vec3f   front, right, up;   // Eular angle form : TODO quaternion
                // = u    = -v 

    Vec3f   defocus_disk_u;     // Defocus disk horizontal radius
    Vec3f   defocus_disk_v;

    void initialize();

    Ray getRay(int pixel_x, int pixel_y) const;
    Vec3f genSampleSquare() const;
    Vec3f genSampleDeforceDisk() const;

    Color getRayColor(const Ray& r, int depth, const Shape& world) const;
};

#endif
