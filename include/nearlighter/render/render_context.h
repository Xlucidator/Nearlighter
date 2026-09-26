#ifndef NEARLIGHTER_RENDER_RENDER_CONTEXT_H
#define NEARLIGHTER_RENDER_RENDER_CONTEXT_H

#include <nearlighter/light/light_sampler.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>

class Scene;

/**
 * Borrowed Render Job Inputs
 *
 * The caller keeps Scene alive; Renderer keeps per-job state alive until
 * integration returns. Scene owns visibility and lights; camera pixel geometry
 * and selection state belong to the render job. The context never copies Scene.
 */
class RenderContext {
public:
    RenderContext(const Scene& scene, const RenderSettings& settings,
                  const Camera::Prepared& camera, const LightSampler& light_sampler)
        : scene_(scene), settings_(settings), camera_(camera),
          light_sampler_(light_sampler) {}

    const Scene& scene() const { return scene_; }
    const RenderSettings& settings() const { return settings_; }
    const Camera::Prepared& camera() const { return camera_; }
    const LightSampler& lightSampler() const { return light_sampler_; }

private:
    const Scene& scene_;
    const RenderSettings& settings_;
    const Camera::Prepared& camera_;
    const LightSampler& light_sampler_;
};

#endif  // NEARLIGHTER_RENDER_RENDER_CONTEXT_H
