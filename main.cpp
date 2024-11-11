#include "vec3f.h"
#include "color.h"
#include "ray.h"

#include "camera.cpp"
#include "shape.h"
#include "shapelist.h"
#include "sphere.h"

#include <iostream>

int main() {
    /* World */
    ShapeList world;
    world.add(make_shared<Sphere>(Point3f(0, 0, -1), 0.5));
    world.add(make_shared<Sphere>(Point3f(0, -100.5, -1), 100));

    /* Camera */
    Camera camera;
    camera.aspect_ratio = 16.0 / 9.0;
    camera.image_width  = 1280;

    /* Render */
    camera.render(world);

    return 0;
}