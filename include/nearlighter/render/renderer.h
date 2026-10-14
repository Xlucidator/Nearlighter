#ifndef NEARLIGHTER_RENDER_RENDERER_H
#define NEARLIGHTER_RENDER_RENDERER_H

#include <nearlighter/render/integrator.h>
#include <nearlighter/render/render_settings.h>

#include <chrono>

class Scene;

/** Render Timings and Path Counters */
struct RenderStats {
    std::chrono::duration<double> camera_preparation_time{};
    std::chrono::duration<double> light_sampler_preparation_time{};
    std::chrono::duration<double> film_allocation_time{};

    /** Per-render setup only; Scene construction is measured separately. */
    std::chrono::duration<double> preparation_time{};
    std::chrono::duration<double> integration_time{};

    IntegratorCounters counters;

    double samplesPerSecond() const {
        if (integration_time.count() <= 0.0) return 0.0;
        return static_cast<double>(counters.sample_count) / integration_time.count();
    }

    double meanPathLength() const {
        if (counters.sample_count == 0) return 0.0;
        return static_cast<double>(counters.path_length_sum) /
               static_cast<double>(counters.sample_count);
    }
};

/** Completed Film and Render Metrics */
struct RenderResult {
    Film film;
    RenderStats stats;

    /** Convenience view of the Film beauty layer. */
    const Image& image() const { return film.beauty(); }
};

/**
 * Render Job Preparation and Dispatch
 *
 * Responsibilities:
 * - Owns job configuration and prepares camera, light selector, and Film.
 * - Reuses the assembled Scene world and lights across render jobs.
 * - Delegates light transport to the Integrator.
 */
class Renderer {
public:
    explicit Renderer(RenderSettings settings = {});

    const RenderSettings& settings() const { return settings_; }

    /**
     * Deterministic Integration into a Typed Film
     *
     * Uses this Renderer's settings; Scene defaults are not applied implicitly.
     * Pass scene.defaultRenderSettings() at construction to adopt those defaults.
     */
    RenderResult render(
        const Scene& scene,
        RenderProgressCallback progress_callback = {}) const;

private:
    RenderSettings settings_;
};

#endif  // NEARLIGHTER_RENDER_RENDERER_H
