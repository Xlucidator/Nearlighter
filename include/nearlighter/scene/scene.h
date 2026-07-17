#ifndef NEARLIGHTER_SCENE_SCENE_H
#define NEARLIGHTER_SCENE_SCENE_H

#include <nearlighter/base/color.h>
#include <nearlighter/geometry/shape_list.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>

#include <string>

/**
 * Owns the complete immutable runtime state required to render one scene.
 *
 * Shapes and materials use shared ownership internally, so moving a Scene is
 * inexpensive even when it contains many objects.
 */
class Scene {
public:
    /** Constructs a complete runtime scene from already-created core objects. */
    Scene(std::string name, Camera camera,
          RenderSettings default_render_settings, Color background,
          ShapeList world, ShapeList sampling_targets = {});

    /** Returns the human-readable scene name. */
    const std::string& name() const { return name_; }

    /** Returns the scene camera parameters. */
    const Camera& camera() const { return camera_; }

    /** Returns the scene's reproducible default render settings. */
    const RenderSettings& defaultRenderSettings() const {
        return default_render_settings_;
    }

    /** Returns the linear environment color used when rays miss the world. */
    const Color& background() const { return background_; }

    /** Returns all shapes participating in ray intersection. */
    const ShapeList& world() const { return world_; }

    /** Returns shapes explicitly used to guide importance sampling. */
    const ShapeList& samplingTargets() const { return sampling_targets_; }

private:
    std::string name_;
    Camera camera_;
    RenderSettings default_render_settings_;
    Color background_;
    ShapeList world_;
    ShapeList sampling_targets_;
};

#endif  // NEARLIGHTER_SCENE_SCENE_H
