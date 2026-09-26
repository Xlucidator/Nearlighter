#include "builtin_generator.h"

#include <nearlighter/accel/bvh.h>
#include <nearlighter/material/dielectric.h>
#include <nearlighter/material/emissive.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/material/metal.h>
#include <nearlighter/math/math.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/instance.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/shape/box.h>
#include <nearlighter/shape/quad.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/texture/checker_texture.h>
#include <nearlighter/texture/noise_texture.h>
#include <nearlighter/geometry/transform.h>

#include <memory>
#include <utility>

namespace builtin_scenes {
namespace {

std::shared_ptr<Primitive> makePrimitive(
    std::shared_ptr<const Shape> shape,
    std::shared_ptr<const Material> material,
    Transform local_to_parent = {}) {
    return std::make_shared<Primitive>(std::move(shape),
                                       std::move(material),
                                       std::move(local_to_parent));
}

}  // namespace

/**
 * @par Implementation
 * - A seeded Sampler controls grid placement and material parameters.
 * - The ground and three principal spheres remain deterministic.
 * - Each accepted grid cell appends one Primitive in generation order.
 */
LinearAggregate generateBouncingSpheres(
    const BouncingSpheresConfig& config) {
    LinearAggregate world;
    Sampler sampler(config.seed);

    /* ----- Ground ----- */
    auto checker = std::make_shared<CheckerTexture>(
        0.32f, Color(0.2f, 0.3f, 0.1f), Color(0.9f, 0.9f, 0.9f));
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(0.0f, -1000.0f, 0.0f),
                                 1000.0f),
        std::make_shared<Lambertian>(checker)));

    /* ----- Random Sphere Grid ----- */
    for (int x = -config.grid_size; x < config.grid_size; ++x) {
        for (int z = -config.grid_size; z < config.grid_size; ++z) {
            const float material_choice = sampler.next1D();
            const Point3f center(
                x + config.spacing * sampler.next1D(), config.radius,
                z + config.spacing * sampler.next1D());
            if ((center - Point3f(4.0f, config.radius, 0.0f)).length() <=
                config.spacing) {
                continue;
            }

            if (material_choice < 0.8f) {
                const Color albedo =
                    sampler.nextVec3() * sampler.nextVec3();
                const Point3f center_end = center + Vec3f(
                    0.0f, sampler.next1D(0.0f, 0.5f), 0.0f);
                world.add(makePrimitive(
                    std::make_shared<Sphere>(center, center_end,
                                             config.radius),
                    std::make_shared<Lambertian>(albedo)));
            } else if (material_choice < 0.95f) {
                const Color albedo = sampler.nextVec3(0.5f, 1.0f);
                const float fuzz = sampler.next1D(0.0f, 0.5f);
                world.add(makePrimitive(
                    std::make_shared<Sphere>(center, config.radius),
                    std::make_shared<Metal>(albedo, fuzz)));
            } else {
                world.add(makePrimitive(
                    std::make_shared<Sphere>(center, config.radius),
                    std::make_shared<Dielectric>(1.5f)));
            }
        }
    }

    /* ----- Main Spheres ----- */
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(0.0f, 1.0f, 0.0f), 1.0f),
        std::make_shared<Dielectric>(1.5f)));
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(-4.0f, 1.0f, 0.0f), 1.0f),
        std::make_shared<Lambertian>(Color(0.4f, 0.2f, 0.1f))));
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(4.0f, 1.0f, 0.0f), 1.0f),
        std::make_shared<Metal>(Color(0.7f, 0.6f, 0.5f), 0.0f)));
    return world;
}

/**
 * @par Implementation
 * - Repeated groups receive private BVHs before top-level insertion.
 * - The sphere cluster is built and accelerated once.
 * - One Instance places the cluster without rebuilding child nodes or bounds.
 */
LinearAggregate generateFinalScene(
    const FinalSceneConfig& config,
    const std::shared_ptr<Texture>& earth_texture) {
    LinearAggregate world;
    Sampler sampler(config.seed);

    /* ----- Ground Box Grid ----- */
    // Uneven green floor: a 20-by-20 grid at the default configuration.
    auto ground_material =
        std::make_shared<Lambertian>(Color(0.48f, 0.83f, 0.53f));
    LinearAggregate ground_boxes;
    constexpr float kCellSize = 100.0f;
    for (int x = 0; x < config.ground_grid_size; ++x) {
        for (int z = 0; z < config.ground_grid_size; ++z) {
            const float x0 = -1000.0f + x * kCellSize;
            const float z0 = -1000.0f + z * kCellSize;
            const float y1 = sampler.next1D(1.0f, 101.0f);
            ground_boxes.add(makePrimitive(
                std::make_shared<Box>(
                    Point3f(x0, 0.0f, z0),
                    Point3f(x0 + kCellSize, y1, z0 + kCellSize)),
                ground_material));
        }
    }
    world.add(std::make_shared<BVH>(ground_boxes));

    /* ----- Lighting and Surface Objects ----- */
    // Quad ceiling light: +X and +Z edges give a downward-facing emitting side
    world.add(makePrimitive(
        std::make_shared<Quad>(
            Point3f(123.0f, 554.0f, 147.0f),
            Vec3f(300.0f, 0.0f, 0.0f),
            Vec3f(0.0f, 0.0f, 265.0f)),
        std::make_shared<Emissive>(Color(7.0f, 7.0f, 7.0f))));

    // Upper-left brown sphere: motion along +X produces shutter blur.
    const Point3f moving_start(400.0f, 400.0f, 200.0f);
    world.add(makePrimitive(
        std::make_shared<Sphere>(
            moving_start, moving_start + Vec3f(30.0f, 0.0f, 0.0f),
            50.0f),
        std::make_shared<Lambertian>(Color(0.7f, 0.3f, 0.1f))));

    // Small dielectric sphere near the bottom center.
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(260.0f, 150.0f, 45.0f), 50.0f),
        std::make_shared<Dielectric>(1.5f)));

    // Gray sphere at the lower right: rough metal with a broad reflection.
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(0.0f, 150.0f, 145.0f), 50.0f),
        std::make_shared<Metal>(Color(0.8f, 0.8f, 0.9f), 1.0f)));

    /* ----- Bounded Media ----- */
    // Dielectric sphere in the lower-left forground with dark blue medium inside.
    auto glass = std::make_shared<Dielectric>(1.5f);
    auto medium_boundary = makePrimitive(
        std::make_shared<Sphere>(Point3f(360.0f, 160.0f, 45.0f), 70.0f),
        glass);
    world.add(medium_boundary);
    world.add(std::make_shared<ConstantMedium>(
        medium_boundary, 0.2f, Color(0.2f, 0.4f, 0.9f)));

    // Sparse white haze surrounds the camera and the entire visible scene.
    medium_boundary = makePrimitive(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, 0.0f), 5000.0f),
        glass);
    world.add(std::make_shared<ConstantMedium>(
        medium_boundary, 0.0001f, Color(1.0f, 1.0f, 1.0f)));

    /* ----- Textured Spheres ----- */
    // Earth sphere at the left: opaque diffuse surface with an image texture.
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(400.0f, 200.0f, 400.0f), 100.0f),
        std::make_shared<Lambertian>(earth_texture)));

    // Large sphere near the image center: procedural marble on a diffuse surface.
    world.add(makePrimitive(
        std::make_shared<Sphere>(Point3f(220.0f, 280.0f, 300.0f), 80.0f),
        std::make_shared<Lambertian>(
            std::make_shared<NoiseTexture>(0.2f))));

    /* ----- Instanced Sphere Cluster ----- */
    // White spher cluster at the upper right filling in a cubic shape
    LinearAggregate cluster;
    auto cluster_material =
        std::make_shared<Lambertian>(Color(0.73f, 0.73f, 0.73f));
    for (int index = 0; index < config.cluster_sphere_count; ++index) {
        cluster.add(makePrimitive(
            std::make_shared<Sphere>(sampler.nextVec3(0.0f, 165.0f),
                                     10.0f),
            cluster_material));
    }

    auto cluster_bvh = std::make_shared<BVH>(cluster);
    // Rotate the whole cluster 15 degrees around Y, then translate it.
    const Transform cluster_to_parent =
        Transform::translate(Vec3f(-100.0f, 270.0f, 395.0f)) *
        Transform::rotate(Vec3f(0.0f, 1.0f, 0.0f),
                          degrees_to_radians(15.0f));
    world.add(std::make_shared<Instance>(cluster_bvh, cluster_to_parent));
    return world;
}

}  // namespace builtin_scenes
