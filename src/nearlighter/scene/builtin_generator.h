#ifndef NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H
#define NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H

#include <nearlighter/geometry/shape_list.h>

#include <cstdint>
#include <memory>

class Texture;

namespace builtin_scenes {

// ==================================================
// Generator Configuration
// ==================================================

/** Parameters controlling the randomized sphere grid. */
struct BouncingSpheresConfig {
    std::uint64_t seed = 0;
    int grid_size = 8;
    float radius = 0.2f;
    float spacing = 0.9f;
};

/** Parameters controlling the two large generated object groups. */
struct FinalSceneConfig {
    std::uint64_t seed = 0;
    int ground_grid_size = 20;
    int cluster_sphere_count = 1000;
};

// ==================================================
// Built-in Generators
// ==================================================

/** Builds the randomized sphere field from deterministic parameters. */
ShapeList generateBouncingSpheres(const BouncingSpheresConfig& config);

/** Builds the RTOW final scene from deterministic parameters and resources. */
ShapeList generateFinalScene(
    const FinalSceneConfig& config,
    const std::shared_ptr<Texture>& earth_texture);

}  // namespace builtin_scenes

#endif  // NEARLIGHTER_SCENE_BUILTIN_GENERATOR_H
