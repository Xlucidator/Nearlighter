#include <nearlighter/scene/scene.h>

#include <utility>

Scene::Scene(std::string name, Camera camera,
             RenderSettings default_render_settings, Color background,
             ShapeList world, ShapeList sampling_targets)
    : name_(std::move(name)),
      camera_(std::move(camera)),
      default_render_settings_(default_render_settings),
      background_(background),
      world_(std::move(world)),
      sampling_targets_(std::move(sampling_targets)) {}
