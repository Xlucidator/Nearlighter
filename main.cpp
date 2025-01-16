#include "common.h"

#include "camera/camera.h"
#include "shape/shapelist.h"
#include "shape/shape.h"
#include "shape/sphere.h"

#include <iostream>

int main() {
    /* World */
    ShapeList world;
    world.add(make_shared<Sphere>(Point3f(0.0f, 0.0f, -1.0f), 0.5f));
    world.add(make_shared<Sphere>(Point3f(0.0f, -100.5f, -1.0f), 100.0f));

    /* Camera */
    Camera camera;
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width  = 400;
    camera.samples_per_pixel = 20;
    camera.max_depth = 20;

    /* Render */
    camera.render(world);

    return 0;
}