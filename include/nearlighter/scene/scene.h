#ifndef NEARLIGHTER_SCENE_SCENE_H
#define NEARLIGHTER_SCENE_SCENE_H

#include <nearlighter/base/color.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/scene/linear_aggregate.h>
#include <nearlighter/scene/primitive.h>

#include <memory>
#include <string>
#include <vector>

/**
 * Render Scene
 *
 * Owns the runtime description consumed by Renderer.
 * - Camera, render defaults, and background define global render state.
 * - World contains every top-level Intersectable used for visibility.
 * - Sampling targets identify surface Primitives used for explicit direction
 *   sampling and are retained by shared ownership.
 * - World storage remains linear and preserves top-level insertion order.
 * - Renderer may build render-local acceleration without mutating Scene.
 */
class Scene {
public:
    /**
     * Scene Construction
     *
     * Takes ownership of the supplied value state and shared sampling targets.
     *
     * @param name Human-readable scene name.
     * @param camera Camera configuration independent of image resolution.
     * @param default_render_settings Reproducible defaults for Renderer setup.
     * @param background Linear environment radiance returned on a world miss.
     * @param world Top-level visibility entities in insertion order.
     * @param sampling_targets Optional explicit direction-sampling targets.
     * The vector may be empty.
     * Every entry must be non-null and report `hasPDF() == true`.
     */
    Scene(std::string name, Camera camera,
          RenderSettings default_render_settings, Color background,
          LinearAggregate world,
          std::vector<std::shared_ptr<const Primitive>> sampling_targets = {});

    /** @name Global Render State
     * Returns immutable scene metadata and camera configuration.
     * @{ */
    const std::string& name() const { return name_; }
    const Camera& camera() const { return camera_; }
    const RenderSettings& defaultRenderSettings() const {
        return default_render_settings_;
    }
    const Color& background() const { return background_; }
    /** @} */

    /** @name Render Entities
     * Returns the visibility world and explicit surface-sampling targets.
     * @{ */
    const LinearAggregate& world() const { return world_; }
    const std::vector<std::shared_ptr<const Primitive>>& samplingTargets()
        const {
        return sampling_targets_;
    }
    /** @} */

private:
    std::string name_;
    Camera camera_;
    RenderSettings default_render_settings_;
    Color background_;
    LinearAggregate world_;
    std::vector<std::shared_ptr<const Primitive>> sampling_targets_;
};

#endif  // NEARLIGHTER_SCENE_SCENE_H
