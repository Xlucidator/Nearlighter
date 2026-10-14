#ifndef NEARLIGHTER_RENDER_RENDER_CONTEXT_H
#define NEARLIGHTER_RENDER_RENDER_CONTEXT_H

#include <nearlighter/render/light_sampler.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>

class Scene;

/**
 * Borrowed Render Job Inputs
 *
 * Lifetime and ownership:
 * - The caller keeps Scene alive until integration returns.
 * - Renderer owns the prepared camera and light selector for this job.
 * - Context borrows all inputs; it copies neither Scene nor settings.
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
