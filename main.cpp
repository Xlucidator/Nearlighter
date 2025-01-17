#include "common.h"

#include "camera/camera.h"
#include "shape/shapelist.h"
#include "material/material.h"

int main() {
    /* Configure */
    initGammaLUT(2.2f);

    /* Material */
    auto mat_ground = make_shared<Lambertian>(Color(0.8f, 0.8f, 0.0f));
    auto mat_center = make_shared<Lambertian>(Color(0.1f, 0.2f, 0.5f));
    // auto mat_left   = make_shared<Metal>(Color(0.8f, 0.8f, 0.8f), 0.1f);
    auto mat_left   = make_shared<Dielectric>(1.50);
    auto mat_right  = make_shared<Metal>(Color(0.8f, 0.6f, 0.2f), 0.8f);

    /* World */
    ShapeList world;
    world.add(make_shared<Sphere>(Point3f( 0.0f, -100.5f, -1.0f), 100.0f, mat_ground));
    world.add(make_shared<Sphere>(Point3f( 0.0f,    0.0f, -1.2f),   0.5f, mat_center));
    world.add(make_shared<Sphere>(Point3f(-1.0f,    0.0f, -1.0f),   0.5f, mat_left));
    world.add(make_shared<Sphere>(Point3f( 1.0f,    0.0f, -1.0f),   0.5f, mat_right));

    /* Camera */
    Camera camera;
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width  = 400;
    camera.samples_per_pixel = 20;
    camera.max_depth = 25;

    /* Render */
    camera.render(world);

    return 0;
}