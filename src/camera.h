#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"
#include "color.h"
#include "shape.h"

class Camera {
public:
    float	aspect_ratio = 1.0;	// Ratio of image width over height
    int 	image_width = 100;	// Rendered image width

    void render(const Shape& world);

private:
    int     image_height;		// Rendered image height
    Point3f center;				// Camera center loaction in world coordinate
    Point3f pixel_start;		// Location of pixel[0][0] in world coordinate
    Vec3f   pixel_interval_u;	// Intervals between pixel in u direction in world coordinate
    Vec3f   pixel_interval_v;	// Intervals between pixel in v direction in world coordinate

    void initialize();
    Color ray_color(const Ray& r, const Shape& world) const;
};

#endif
