#ifndef NEARLIGHTER_SCENE_SCENE_H
#define NEARLIGHTER_SCENE_SCENE_H

#include <nearlighter/light/light.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/scene/linear_aggregate.h>
#include <nearlighter/scene/primitive.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class AreaLight;
class EnvironmentLight;
class Material;
class Transform;

/** Scene Construction Timings */
struct SceneBuildStats {
    // Includes rebuilt inner aggregates.
    std::chrono::duration<double> assembly_time{};
    // Includes only the final world root.
    std::chrono::duration<double> acceleration_build_time{};
    std::chrono::duration<double> light_build_time{};
};

/**
 * Assembled Render Scene
 *
 * Resource ownership:
 * - Owns the assembled world and lights.
 * - Shares source geometry and materials.
 * - Requires shared resources to remain unchanged.
 */
class Scene {
public:
    /**
     * @name Construction
     * @{
     */
    /**
     * Scene Assembly
     *
     * Construction guarantees:
     * - Preserves input objects.
     * - Completes world and light construction before returning.
     * - Gives each sampleable emissive placement an AreaLight.
     *
     * @param background Constant environment radiance; black omits the environment.
     * @param world Acyclic hierarchy with stable shared content and bounds.
     * @param sampling_targets Non-null, sampleable legacy proposals.
     * @throws std::invalid_argument For invalid targets or detected cycles.
     */
    Scene(std::string name, Camera camera,
          RenderSettings default_render_settings, Color background,
          LinearAggregate world,
          std::vector<std::shared_ptr<const Primitive>> sampling_targets = {});

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;
    /** @} */

    /**
     * @name Scene Configuration
     * Camera and settings are defaults for render jobs.
     * @{
     */
    const std::string& name() const { return name_; }
    const Camera& camera() const { return camera_; }
    const RenderSettings& defaultRenderSettings() const {
        return default_render_settings_;
    }
    /** @} */

    /**
     * @name World and Light Queries
     * Light pointers borrow from this Scene.
     * @{
     */
    const Intersectable& world() const { return *world_; }
    const std::vector<std::unique_ptr<Light>>& lights() const { return lights_; }

    /** Null for a black background. */
    const EnvironmentLight* environment() const { return environment_; }

    /**
     * Surface-to-Light Lookup
     *
     * @param primitive Surface returned by a hit in world().
     * @return The AreaLight used for MIS, or null if none was built.
     */
    const AreaLight* findAreaLights(const Primitive& primitive) const;
    /** @} */

    /**
     * @name Integrator Compatibility Queries
     * Reports inspected content for integrator validation.
     * ConstantMedium boundaries are excluded from surface inspection.
     * @{
     */
    /** Deduplicated material references. */
    const std::vector<const Material*>& materials() const { return materials_; }

    /** Number of emissive placements without a sampling distribution. */
    std::size_t unsampleableEmitterCount() const {
        return unsampleable_emitter_count_;
    }
    bool containsMedia() const { return contains_media_; }

    /** False if inspection encountered an unknown Intersectable. */
    bool inspectionComplete() const { return inspection_complete_; }
    /** @} */

    /** Construction costs, excluding per-render preparation. */
    const SceneBuildStats& buildStats() const { return build_stats_; }

    /**
     * Legacy Direction Proposals
     *
     * Independent of light discovery:
     * - Emissive targets expand to their scene placements.
     * - Other targets remain unchanged.
     */
    const std::vector<std::shared_ptr<const Primitive>>& samplingTargets() const {
        return sampling_targets_;
    }

private:
    /* Scene construction. */
    struct BuildCache;

    /**
     * Emissive Primitive Extraction
     *
     * Extraction contract:
     * - Preserves input objects and shares their Shape and Material.
     * - Records surface materials and scene compatibility facts.
     *
     * @param object Root of the input subtree.
     * @param parent_to_world Maps the object's parent space into world space.
     * @param emissive_primitives Appends one world-space Primitive per emissive placement.
     * @param cache Reuses source remainders and remaps legacy targets.
     * @return The non-emissive remainder, or null when all content was extracted.
     */
    std::shared_ptr<const Intersectable> extractEmissivePrimitives(
        const std::shared_ptr<const Intersectable>& object,
        const Transform& parent_to_world,
        std::vector<std::shared_ptr<const Primitive>>& emissive_primitives,
        BuildCache& cache);

    /**
     * Scene Light Construction
     *
     * Construction results:
     * - AreaLights and their surface lookup entries.
     * - Unsampleable emitter count for integrator validation.
     * - Constant environment light for a nonzero background.
     *
     * @param emissive_primitives World-space emitters already owned by world_.
     */
    void buildLights(
        const std::vector<std::shared_ptr<const Primitive>>& emissive_primitives,
        const Color& background);

    /* Scene configuration. */
    std::string name_;
    Camera camera_;
    RenderSettings default_render_settings_;

    /* World ownership and compatibility facts. */
    std::shared_ptr<const Intersectable> world_;
    std::vector<const Material*> materials_;
    std::size_t unsampleable_emitter_count_ = 0;
    bool contains_media_ = false;
    bool inspection_complete_ = true;

    /* Lights borrow scene Primitives. */
    std::vector<std::unique_ptr<Light>> lights_;
    // Lookup pointers borrow from lights_.
    std::unordered_map<const Primitive*, const AreaLight*> primitive_to_arealights_;
    const EnvironmentLight* environment_ = nullptr;

    /* Legacy proposals and construction timings. */
    std::vector<std::shared_ptr<const Primitive>> sampling_targets_;
    SceneBuildStats build_stats_;
};

#endif  // NEARLIGHTER_SCENE_SCENE_H
