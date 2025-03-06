#include "common.h"

#include "camera/camera.h"
#include "shape/shapelist.h"
#include "material/material.h"
#include "medium/constant_medium.h"
#include "bvh/bvhnode.h"
#include "texture/texture.h"
#include "utils/timer.h"

#include <fstream>
#include <chrono>

void validate() {
    std::cout << "infinity = " << infinity << std::endl
              << "empty interval: " << Interval::empty << std::endl
              << "universe interval: " << Interval::universe << std::endl;
    std::cout << "empty aabb: " << AABB::empty() << std::endl
              << "universe aabb: " << AABB::universe() << std::endl;
}

void set_scenery_boucingSpheres(ShapeList& world, Camera& camera, bool is_random = true);
void set_scenery_checkerSpheres(ShapeList& world, Camera& camera);
void set_scenery_earth(ShapeList& world, Camera& camera);
void set_scenery_perlinSphere(ShapeList& world, Camera& camera);
void set_scenery_quads(ShapeList& world, Camera& camera);
void set_scenery_simpleLight(ShapeList& world, Camera& camera);
void set_scenery_CornellBox(ShapeList& world, Camera& camera);
void set_scenery_CornellSmoke(ShapeList& world, Camera& camera);

int main(int argc, char* argv[]) {
    /* Configure */
    initGammaLUT(2.2f);
    if constexpr (false) validate();

    /* Scenery */
    ShapeList world;
    Camera camera;
    // Output Viewport settings
    camera.aspect_ratio = 16.0f / 9.0f;
    camera.image_width  = 400;     // 400
    camera.samples_per_pixel = 30; // 30
    camera.max_depth = 25;         // 25
    // Scenery Display settings
    int select = argc > 2 ? std::stoi(argv[2]) : 0; // temporary '-s | --scene <number>'
    switch (select) {
        case 0: set_scenery_boucingSpheres(world, camera); break;
        case 1: set_scenery_checkerSpheres(world, camera); break;
        case 2: set_scenery_earth(world, camera); break;
        case 3: set_scenery_perlinSphere(world, camera); break;
        case 4: set_scenery_quads(world, camera); break;
        case 5: set_scenery_simpleLight(world, camera); break;
        case 6: set_scenery_CornellBox(world, camera); break;
        case 7: set_scenery_CornellSmoke(world, camera); break;
        default: break;
    }

    /* Acceleration */
    BVHNode bvh_root = BVHNode(world);
    // std::cout << "object size = " << world.size() << std::endl;
    // bvh_root.printNode(0);

    /* Render */
    std::ofstream ouput_file("out.ppm");
    if (!ouput_file) {
        std::cerr << "ERROR: Failed to Open out.ppm\n";
        return -1;
    }
    Timer timer("Render");
    camera.render(bvh_root, ouput_file);
    timer.reply();
    ouput_file.close();

    return 0;
}


void set_scenery_boucingSpheres(ShapeList& world, Camera& camera, bool is_random) {
    /* Objects Display */
    // Gound
    auto checker = make_shared<CheckerTexture>(0.32, Color(0.2, 0.3, 0.1), Color(0.9, 0.9, 0.9));
    auto mat_ground = make_shared<Lambertian>(checker);
    world.add(make_shared<Sphere>(Point3f(0, -1000, 0), 1000, mat_ground));
    // (Random) Balls Grid
    int grid_size = 8;
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

    /* Camera Parameters */
    // Extrinsic
    camera.fov_vertical = 20.0;
    camera.position = Point3f(13, 2, 3);    // Point3f(-2, 2,  1);
    camera.look_at  = Point3f(0, 0, 0);     // Point3f( 0, 0, -1);
    camera.world_up = Vec3f(0, 1, 0);
    // Intrinsic
    camera.defocus_angle = 0.6;
    camera.focus_distance = 10.0;
}

void set_scenery_checkerSpheres(ShapeList& world, Camera& camera) {
    /* Objects Display */
    auto checker = make_shared<CheckerTexture>(0.32, Color(0.2, 0.3, 0.1), Color(0.9, 0.9, 0.9));
    world.add(make_shared<Sphere>(Point3f(0,-10, 0), 10, make_shared<Lambertian>(checker)));
    world.add(make_shared<Sphere>(Point3f(0, 10, 0), 10, make_shared<Lambertian>(checker)));

    /* Camera Parameters */
    // Extrinsic
    camera.fov_vertical = 20.0;
    camera.position = Point3f(13, 2, 3);
    camera.look_at  = Point3f(0, 0, 0);     // Point3f( 0, 0, -1);
    camera.world_up = Vec3f(0, 1, 0);
    // Intrinsic
}

void set_scenery_earth(ShapeList& world, Camera& camera) {
    initGammaLUT(1.0);  // for i use stb_load, which return color in sRGB that has been gamma corrected
    // TODO: use stbi_loadf + convertion, which return color in linear space

    /* Objects Display */
    auto earth_texture = make_shared<ImageTexture>("earthmap.png");
    auto earth_material = make_shared<Lambertian>(earth_texture);
    world.add(make_shared<Sphere>(Point3f(0, 0, 0), 2, earth_material));

    /* Camera Parameters */
    // Extrinsic
    camera.fov_vertical = 20.0;
    camera.position = Point3f(0, 0, 12);
    camera.look_at  = Point3f(0, 0, 0); 
    // Intrinsic
}

void set_scenery_perlinSphere(ShapeList& world, Camera& camera) {
    auto perlin_texture = make_shared<NoiseTexture>(4);
    world.add(make_shared<Sphere>(Point3f(0, -1000, 0), 1000, make_shared<Lambertian>(perlin_texture)));
    world.add(make_shared<Sphere>(Point3f(0, 2, 0), 2, make_shared<Lambertian>(perlin_texture)));

    /* Camera Parameters */
    // Extrinsic
    camera.fov_vertical = 20.0;
    camera.position = Point3f(13, 2, 3);
    camera.look_at  = Point3f(0, 0, 0);
    // Intrinsic
}

void set_scenery_quads(ShapeList& world, Camera& camera) {
    /* Objects Display */
    auto left_red     = make_shared<Lambertian>(Color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<Lambertian>(Color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<Lambertian>(Color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<Lambertian>(Color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<Lambertian>(Color(0.2, 0.8, 0.8));

    world.add(make_shared<Quad>(Point3f(-3,-2, 5), Vec3f(0, 0,-4), Vec3f(0, 4, 0), left_red));
    world.add(make_shared<Quad>(Point3f(-2,-2, 0), Vec3f(4, 0, 0), Vec3f(0, 4, 0), back_green));
    world.add(make_shared<Quad>(Point3f( 3,-2, 1), Vec3f(0, 0, 4), Vec3f(0, 4, 0), right_blue));
    world.add(make_shared<Quad>(Point3f(-2, 3, 1), Vec3f(4, 0, 0), Vec3f(0, 0, 4), upper_orange));
    world.add(make_shared<Quad>(Point3f(-2,-3, 5), Vec3f(4, 0, 0), Vec3f(0, 0,-4), lower_teal));

    /* Camera Parameters */
    camera.aspect_ratio = 1.0;
    // Extrinsic
    camera.fov_vertical = 80;
    camera.position = Point3f(0, 0, 9);
    camera.look_at  = Point3f(0, 0, 0);
    // Intrinsic
}

void set_scenery_simpleLight(ShapeList& world, Camera& camera) {
    /* Objects Display */
    auto perlin_texture = make_shared<NoiseTexture>(4);
    world.add(make_shared<Sphere>(Point3f(0, -1000, 0), 1000, make_shared<Lambertian>(perlin_texture)));
    world.add(make_shared<Sphere>(Point3f(0, 2, 0), 2, make_shared<Lambertian>(perlin_texture)));

    auto light = make_shared<DiffuseLight>(Color(0.9, 0.9, 0.4)); // has exceed the meaning of Color, could be over (1, 1, 1)
    world.add(make_shared<Sphere>(Point3f(0,7,0), 2, light));
    world.add(make_shared<Quad>(Point3f(3, 1, -2), Vec3f(2, 0, 0), Vec3f(0, 2, 0), light));
    
    /* Camera Parameters */
    camera.background = Color(0, 0, 0);
    // Extrinsic
    camera.fov_vertical = 20.0;
    camera.position = Point3f(26, 3, 6);
    camera.look_at  = Point3f(0, 2, 0);
    // Intrinsic
}

void set_scenery_CornellBox(ShapeList& world, Camera& camera) {
    /* Objects Display */
    auto red   = make_shared<Lambertian>(Color(.65, .05, .05));
    auto white = make_shared<Lambertian>(Color(.73, .73, .73));
    auto green = make_shared<Lambertian>(Color(.12, .45, .15));
    auto light = make_shared<DiffuseLight>(Color(15, 15, 15));

    // Environment
    world.add(make_shared<Quad>(Point3f(555, 0  , 0  ), Vec3f( 0  , 555, 0), Vec3f(0, 0  ,  555), green));
    world.add(make_shared<Quad>(Point3f(0  , 0  , 0  ), Vec3f( 0  , 555, 0), Vec3f(0, 0  ,  555), red  ));
    world.add(make_shared<Quad>(Point3f(343, 554, 332), Vec3f(-130, 0  , 0), Vec3f(0, 0  , -105), light));
    world.add(make_shared<Quad>(Point3f(0  , 0  , 0  ), Vec3f( 555, 0  , 0), Vec3f(0, 0  ,  555), white));
    world.add(make_shared<Quad>(Point3f(555, 555, 555), Vec3f(-555, 0  , 0), Vec3f(0, 0  , -555), white));
    world.add(make_shared<Quad>(Point3f(0  , 0  , 555), Vec3f( 555, 0  , 0), Vec3f(0, 555,  0  ), white));
    // Boxes
    shared_ptr<Shape> box1 = box(Point3f(0, 0, 0), Point3f(165, 330, 165), white);
    box1 = make_shared<Rotate>(box1, Vec3f(0, 1, 0), degrees_to_radians(15));
    box1 = make_shared<Translate>(box1, Vec3f(265, 0, 295));
    world.add(box1);
    // world.add(box(Point3f(265, 0, 295), Point3f(430, 330, 460), white));
    
    shared_ptr<Shape> box2 = box(Point3f(0, 0, 0), Point3f(165, 165, 165), white);
    box2 = make_shared<Rotate>(box2, Vec3f(0, 1, 0), degrees_to_radians(-18));
    box2 = make_shared<Translate>(box2, Vec3f(130, 0, 65));
    world.add(box2);
    // world.add(box(Point3f(130, 0, 65 ), Point3f(295, 165, 230), white));

    /* Camera Parameters */
    camera.aspect_ratio = 1.0;
    // camera.image_width  = 600;
    camera.samples_per_pixel = 200;
    camera.max_depth = 50;
    camera.background   = Color(0, 0, 0);
    // Extrinsic
    camera.fov_vertical = 40.0;
    camera.position = Point3f(278, 278, -800);
    camera.look_at  = Point3f(278, 278, 0);
    // Intrinsic
}

void set_scenery_CornellSmoke(ShapeList& world, Camera& camera) {
        /* Objects Display */
        auto red   = make_shared<Lambertian>(Color(.65, .05, .05));
        auto white = make_shared<Lambertian>(Color(.73, .73, .73));
        auto green = make_shared<Lambertian>(Color(.12, .45, .15));
        auto light = make_shared<DiffuseLight>(Color(15, 15, 15));
    
        // Environment
        // world.add(make_shared<Quad>(Point3f(555, 0  , 0  ), Vec3f( 0  , 555, 0), Vec3f(0, 0  ,  555), green));
        // world.add(make_shared<Quad>(Point3f(0  , 0  , 0  ), Vec3f( 0  , 555, 0), Vec3f(0, 0  ,  555), red  ));
        world.add(make_shared<Quad>(Point3f(343, 554, 332), Vec3f(-130, 0  , 0), Vec3f(0, 0  , -105), light));
        world.add(make_shared<Quad>(Point3f(0  , 0  , 0  ), Vec3f( 555, 0  , 0), Vec3f(0, 0  ,  555), white));
        world.add(make_shared<Quad>(Point3f(555, 555, 555), Vec3f(-555, 0  , 0), Vec3f(0, 0  , -555), white));
        world.add(make_shared<Quad>(Point3f(0  , 0  , 555), Vec3f( 555, 0  , 0), Vec3f(0, 555,  0  ), white));

        // Smoke Boxes
        shared_ptr<Shape> box1 = box(Point3f(0, 0, 0), Point3f(165, 330, 165), white);
        box1 = make_shared<Rotate>(box1, Vec3f(0, 1, 0), degrees_to_radians(15));
        box1 = make_shared<Translate>(box1, Vec3f(265, 0, 295));
        world.add(make_shared<ConstantMedium>(box1, 0.01, Color(0, 0, 0)));  // Smoke 1
        
        // shared_ptr<Shape> box2 = box(Point3f(0, 0, 0), Point3f(165, 165, 165), white);
        // box2 = make_shared<Rotate>(box2, Vec3f(0, 1, 0), degrees_to_radians(-18));
        // box2 = make_shared<Translate>(box2, Vec3f(130, 0, 65));
        // world.add(make_shared<ConstantMedium>(box2, 0.01, Color(1, 1, 1)));  // Smoke 2
    
        /* Camera Parameters */
        camera.aspect_ratio = 1.0;
        // camera.image_width  = 600;
        camera.samples_per_pixel = 50;
        camera.max_depth = 50;
        camera.background   = Color(0, 0, 0);
        // Extrinsic
        camera.fov_vertical = 40.0;
        camera.position = Point3f(278, 278, -800);
        camera.look_at  = Point3f(278, 278, 0);
        // Intrinsic
}