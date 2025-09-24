#include "scene.h"
#include "utils/timer.h"

#include <fstream>

void validate() {
    std::cout << "infinity = " << infinity << std::endl
              << "empty interval: " << Interval::empty << std::endl
              << "universe interval: " << Interval::universe << std::endl;
    std::cout << "empty aabb: " << AABB::empty() << std::endl
              << "universe aabb: " << AABB::universe() << std::endl;
}

int main(int argc, char* argv[]) {
    /* Configure */
    initGammaLUT(2.2f);
    if constexpr (false) validate();

    /* Scenery */
    ShapeList world;
    Camera camera;
    ShapeList lights;

    // Output Viewport settings
    camera.aspect_ratio      = 16.0 / 9.0;
    camera.image_width       = 400;
    camera.samples_per_pixel = 30;
    camera.max_depth         = 25;
    // Scenery Display settings
    int select = argc > 2 ? std::stoi(argv[2]) : 6; // temporary '-s | --scene <number>'
    switch (select) {
        case 0: set_scenery_boucingSpheres(world, camera); break;
        case 1: set_scenery_checkerSpheres(world, camera); break;
        case 2: set_scenery_earth(world, camera); break;
        case 3: set_scenery_perlinSphere(world, camera); break;
        case 4: set_scenery_quads(world, camera); break;
        case 5: set_scenery_simpleLight(world, camera); break;
        case 6: set_scenery_CornellBox(world, camera, lights, 400, 256, 25); break;
        case 7: set_scenery_CornellSmoke(world, camera); break;
        case 8: set_scenery_finalScene(world, camera, 400, 250, 25); break;
        case 9: set_scenery_CornellBall(world, camera, lights, 400, 256, 25); break;
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
    camera.render(bvh_root, lights, ouput_file);
    timer.reply();
    ouput_file.close();

    return 0;
}
