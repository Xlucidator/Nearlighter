#include "builtin_generator.h"

#include <nearlighter/accel/bvh_node.h>
#include <nearlighter/geometry/quad.h>
#include <nearlighter/geometry/sphere.h>
#include <nearlighter/material/dielectric.h>
#include <nearlighter/material/diffuse_light.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/material/metal.h>
#include <nearlighter/math/math.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/texture/checker_texture.h>
#include <nearlighter/texture/noise_texture.h>
#include <nearlighter/transform/rotate.h>
#include <nearlighter/transform/translate.h>

#include <memory>

namespace builtin_scenes {

// ==================================================
// Built-in Generators
// ==================================================

ShapeList generateBouncingSpheres(const BouncingSpheresConfig& config) {
    ShapeList world;
    Sampler sampler(config.seed);

    /* ----- Ground ----- */
    auto checker = std::make_shared<CheckerTexture>(
        0.32f, Color(0.2f, 0.3f, 0.1f), Color(0.9f, 0.9f, 0.9f));
    auto ground = std::make_shared<Lambertian>(checker);
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, -1000.0f, 0.0f), 1000.0f, ground));

    /* ----- Random Sphere Grid ----- */
    for (int x = -config.grid_size; x < config.grid_size; ++x) {
        for (int z = -config.grid_size; z < config.grid_size; ++z) {
            const float material_choice = sampler.next1D();
            const Point3f center(
                x + config.spacing * sampler.next1D(), config.radius,
                z + config.spacing * sampler.next1D());

            // Reserve space for the main metal sphere.
            if ((center - Point3f(4.0f, config.radius, 0.0f)).length() <=
                config.spacing) {
                continue;
            }

            if (material_choice < 0.8f) {  // Moving diffuse sphere.
                const Color albedo = sampler.nextVec3() * sampler.nextVec3();
                auto material = std::make_shared<Lambertian>(albedo);
                const Point3f center_end =
                    center + Vec3f(0.0f, sampler.next1D(0.0f, 0.5f), 0.0f);
                world.add(std::make_shared<Sphere>(
                    center, center_end, config.radius, material));
            } else if (material_choice < 0.95f) {  // Metal sphere.
                const Color albedo = sampler.nextVec3(0.5f, 1.0f);
                const float fuzz = sampler.next1D(0.0f, 0.5f);
                world.add(std::make_shared<Sphere>(
                    center, config.radius,
                    std::make_shared<Metal>(albedo, fuzz)));
            } else {  // Glass sphere.
                world.add(std::make_shared<Sphere>(
                    center, config.radius,
                    std::make_shared<Dielectric>(1.5f)));
            }
        }
    }

    /* ----- Main Spheres ----- */
    /* Glass sphere. */
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, 1.0f, 0.0f), 1.0f,
        std::make_shared<Dielectric>(1.5f)));

    /* Diffuse sphere. */
    world.add(std::make_shared<Sphere>(
        Point3f(-4.0f, 1.0f, 0.0f), 1.0f,
        std::make_shared<Lambertian>(Color(0.4f, 0.2f, 0.1f))));

    /* Metal sphere. */
    world.add(std::make_shared<Sphere>(
        Point3f(4.0f, 1.0f, 0.0f), 1.0f,
        std::make_shared<Metal>(Color(0.7f, 0.6f, 0.5f), 0.0f)));

    return world;
}

ShapeList generateFinalScene(
    const FinalSceneConfig& config,
    const std::shared_ptr<Texture>& earth_texture) {
    ShapeList world;
    Sampler sampler(config.seed);

    /* ----- Ground ----- */
    auto ground_material =
        std::make_shared<Lambertian>(Color(0.48f, 0.83f, 0.53f));
    ShapeList ground_boxes;
    constexpr float kCellSize = 100.0f;
    for (int x = 0; x < config.ground_grid_size; ++x) {
        for (int z = 0; z < config.ground_grid_size; ++z) {
            const float x0 = -1000.0f + x * kCellSize;
            const float z0 = -1000.0f + z * kCellSize;
            const float y1 = sampler.next1D(1.0f, 101.0f);
            ground_boxes.add(box(
                Point3f(x0, 0.0f, z0),
                Point3f(x0 + kCellSize, y1, z0 + kCellSize),
                ground_material));
        }
    }
    world.add(std::make_shared<BVHNode>(ground_boxes));

    /* ----- Main Objects ----- */
    /* Area light. */
    auto light = std::make_shared<DiffuseLight>(Color(7.0f, 7.0f, 7.0f));
    world.add(std::make_shared<Quad>(
        Point3f(123.0f, 554.0f, 147.0f), Vec3f(300.0f, 0.0f, 0.0f),
        Vec3f(0.0f, 0.0f, 265.0f), light));

    /* Moving diffuse sphere. */
    const Point3f moving_start(400.0f, 400.0f, 200.0f);
    world.add(std::make_shared<Sphere>(
        moving_start, moving_start + Vec3f(30.0f, 0.0f, 0.0f), 50.0f,
        std::make_shared<Lambertian>(Color(0.7f, 0.3f, 0.1f))));

    /* Glass sphere. */
    world.add(std::make_shared<Sphere>(
        Point3f(260.0f, 150.0f, 45.0f), 50.0f,
        std::make_shared<Dielectric>(1.5f)));

    /* Metal sphere. */
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, 150.0f, 145.0f), 50.0f,
        std::make_shared<Metal>(Color(0.8f, 0.8f, 0.9f), 1.0f)));

    /* ----- Participating Media ----- */
    /* Local colored medium and its visible glass boundary. */
    auto medium_boundary = std::make_shared<Sphere>(
        Point3f(360.0f, 160.0f, 45.0f), 70.0f,
        std::make_shared<Dielectric>(1.5f));
    world.add(medium_boundary);
    world.add(std::make_shared<ConstantMedium>(
        medium_boundary, 0.2f, Color(0.2f, 0.4f, 0.9f)));

    /* Global atmospheric medium. */
    medium_boundary = std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, 0.0f), 5000.0f,
        std::make_shared<Dielectric>(1.5f));
    world.add(std::make_shared<ConstantMedium>(
        medium_boundary, 0.0001f, Color(1.0f, 1.0f, 1.0f)));

    /* ----- Textured Spheres ----- */
    /* Earth image texture. */
    world.add(std::make_shared<Sphere>(
        Point3f(400.0f, 200.0f, 400.0f), 100.0f,
        std::make_shared<Lambertian>(earth_texture)));

    /* Perlin noise texture. */
    world.add(std::make_shared<Sphere>(
        Point3f(220.0f, 280.0f, 300.0f), 80.0f,
        std::make_shared<Lambertian>(
            std::make_shared<NoiseTexture>(0.2f))));

    /* ----- Sphere Cluster ----- */
    ShapeList cluster;
    auto cluster_material =
        std::make_shared<Lambertian>(Color(0.73f, 0.73f, 0.73f));
    for (int index = 0; index < config.cluster_sphere_count; ++index) {
        cluster.add(std::make_shared<Sphere>(
            sampler.nextVec3(0.0f, 165.0f), 10.0f, cluster_material));
    }

    /* Shared BVH acceleration and instance transform. */
    auto cluster_bvh = std::make_shared<BVHNode>(cluster);
    auto rotated_cluster = std::make_shared<Rotate>(
        cluster_bvh, Vec3f(0.0f, 1.0f, 0.0f), degrees_to_radians(15.0f));
    world.add(std::make_shared<Translate>(
        rotated_cluster, Vec3f(-100.0f, 270.0f, 395.0f)));

    return world;
}

}  // namespace builtin_scenes
