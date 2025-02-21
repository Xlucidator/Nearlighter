#include "common.h"

#include "camera/camera.h"
#include "shape/shapelist.h"
#include "material/material.h"
#include "bvh/bvhnode.h"

#include <fstream>

void scenery_boucingSphere(bool is_random);

int main() {
    /* Configure */
    initGammaLUT(2.2f);

    /* Debug */
    std::cout << "infinity = " << infinity << std::endl
              << "empty interval: " << Interval::empty << std::endl
              << "universe interval: " << Interval::universe << std::endl;
    std::cout << "empty aabb: " << AABB::empty() << std::endl
              << "universe aabb: " << AABB::universe() << std::endl;

    /* Scenery Select */
    scenery_boucingSphere(false);

    return 0;
}


void scenery_boucingSphere(bool is_random) {
    /* World */
    ShapeList world;

    // Gound
    auto mat_ground = make_shared<Lambertian>(Color(0.5, 0.5, 0.5));
    world.add(make_shared<Sphere>(Point3f(0, -1000, 0), 1000, mat_ground));

    // Random Small Balls
    int grid_size = 11;
    float radius = 0.2, interval = 0.9;
    for (int i = -grid_size; i < grid_size; ++i) {
        for (int j = -grid_size; j < grid_size; ++j) {
            float choise = is_random ? random_float() : (static_cast<float>(i + grid_size) / (grid_size * 2));
            Point3f ball_center = is_random ? Point3f(i + interval * random_float(), radius, j + interval * random_float()) : Point3f(i, radius, j);

            if ((ball_center - Point3f(4, radius, 0)).length() > interval) {
                shared_ptr<Material> ball_material;

                if (choise < 0.8) { // Diffuse Material
                    auto albedo = Color::random() * Color::random();
                    ball_material = make_shared<Lambertian>(albedo);
                    auto move_center_end = ball_center + Vec3f(0, is_random ? random_float(0, 0.5) : 0.25, 0);
                    world.add(make_shared<Sphere>(ball_center, move_center_end, radius, ball_material));
                    continue;
                } else if (choise < 0.95) { // Metal Material
                    auto albedo = Color::random(0.5, 1);
                    auto fuzz = random_float(0, 0.5);
                    ball_material = make_shared<Metal>(albedo, fuzz);
                } else { // Dielectric Material: Glass
                    ball_material = make_shared<Dielectric>(1.5);
                }

                world.add(make_shared<Sphere>(ball_center, radius, ball_material));
            }
        }
    }

    // Main Balls
    auto mat_middle = make_shared<Dielectric>(1.5);
    world.add(make_shared<Sphere>(Point3f(0, 1, 0), 1.0, mat_middle));
    auto mat_back = make_shared<Lambertian>(Color(0.4, 0.2, 0.1));
    world.add(make_shared<Sphere>(Point3f(-4, 1, 0), 1.0, mat_back));
    auto mat_front = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<Sphere>(Point3f(4, 1, 0), 1.0, mat_front));

    // Hollow Glass
    // auto mat_left   = make_shared<Dielectric>(1.5f);
    // auto mat_bubble = make_shared<Dielectric>(1.0f / 1.5f);
    // world.add(make_shared<Sphere>(Point3f(-1.0f,    0.0f, -1.0f),   0.5f, mat_left));   // hollow glass outer
    // world.add(make_shared<Sphere>(Point3f(-1.0f,    0.0f, -1.0f),   0.4f, mat_bubble)); // hollow glass inner

    /* Acceleration */
    std::cout << "Total Object Size: " << world.size() << std::endl;  // should be: 1 + (22 * 22 - 1) + 3 = 487
    BVHNode bvh_root = BVHNode(world);
    bvh_root.printNode(0);

    /* Camera */
    Camera camera;
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width  = 400;     // 400
    camera.samples_per_pixel = 60; // 30
    camera.max_depth = 30;          // 25

    camera.fov_vertical = 20.0f;
    camera.position = Point3f(13, 2, 3);    // Point3f(-2, 2,  1);
    camera.look_at  = Point3f(0, 0, 0);     // Point3f( 0, 0, -1);
    camera.world_up = Vec3f(0, 1, 0);

    camera.defocus_angle = 0.6;
    camera.focus_distance = 10.0;

    /* Render */
    std::ofstream ouput_file("out.ppm");
    if (!ouput_file) {
        std::cerr << "Failed to Open out.ppm\n";
        return;
    }
    camera.render(bvh_root, ouput_file);
    ouput_file.close();
}