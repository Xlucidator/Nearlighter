#ifndef NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H
#define NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H

#include <nearlighter/scene/linear_aggregate.h>

#include <cstdint>
#include <memory>

class Texture;

namespace builtin_scenes {

/** Randomized Sphere-Field Configuration */
struct BouncingSpheresConfig {
    std::uint64_t seed = 0;
    int grid_size = 8;
    float radius = 0.2f;
    float spacing = 0.9f;
};

/** Final-Scene Group Configuration */
struct FinalSceneConfig {
    std::uint64_t seed = 0;
    int ground_grid_size = 20;
    int cluster_sphere_count = 1000;
};

/**
 * Randomized Sphere-Field Generation
 *
 * @param config Deterministic seed, grid extent, sphere radius, and spacing.
 * @return Top-level entities in their generated insertion order.
 */
LinearAggregate generateBouncingSpheres(
    const BouncingSpheresConfig& config);

/**
 * RTOW Final-Scene Generation
 *
 * @param config Deterministic seed and generated group sizes.
 * @param earth_texture Shared texture used by the Earth sphere.
 * @return Top-level entities including internal acceleration and instancing.
 */
LinearAggregate generateFinalScene(
    const FinalSceneConfig& config,
    const std::shared_ptr<Texture>& earth_texture);

}  // namespace builtin_scenes

#endif  // NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H
