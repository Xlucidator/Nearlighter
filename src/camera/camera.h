#ifndef CAMERA_H
#define CAMERA_H

#include "common.h"
#include "shape/shape.h"
#include "material/material.h"

class Camera {
public:
    float	aspect_ratio = 1.0f;// Ratio of image width over height
    int 	image_width = 100;	// Rendered image width
    int     samples_per_pixel = 10; // Render samples for each pixel
    int     max_depth = 10;     // Max bounce times for a ray

    void render(const Shape& world, std::ostream& out = std::cout);

private:
    int     image_height;		// Rendered image height
    float   pixel_samples_scale;// Color scale factor
    Point3f center;				// Camera center location in world coordinate
    Point3f pixel_start;		// Location of pixel[0][0] in world coordinate
    Vec3f   pixel_interval_u;	// Intervals between pixel in u direction in world coordinate
    Vec3f   pixel_interval_v;	// Intervals between pixel in v direction in world coordinate

    void initialize();

    Ray getRay(int pixel_x, int pixel_y) const;
    Vec3f genSampleSquare() const;

    Color getRayColor(const Ray& r, int depth, const Shape& world) const;
};

#endif
