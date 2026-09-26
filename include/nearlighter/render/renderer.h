#ifndef NEARLIGHTER_RENDER_RENDERER_H
#define NEARLIGHTER_RENDER_RENDERER_H

#include <nearlighter/render/integrator.h>

#include <chrono>
#include <cstdint>

class Scene;

/** Preparation, integration, and aggregate ray-work metrics. */
struct RenderStats {
    std::chrono::duration<double> camera_preparation_time{};
    std::chrono::duration<double> light_sampler_preparation_time{};
    std::chrono::duration<double> film_allocation_time{};

    /** Per-render setup only; Scene construction is measured separately. */
    std::chrono::duration<double> preparation_time{};
    std::chrono::duration<double> integration_time{};

    std::uint64_t sample_count = 0;
    std::uint64_t camera_rays = 0;
    std::uint64_t continuation_rays = 0;
    std::uint64_t shadow_rays = 0;
    std::uint64_t surface_interactions = 0;
    std::uint64_t medium_interactions = 0;
    std::uint64_t path_length_sum = 0;
    std::uint64_t max_depth_terminations = 0;
    std::uint64_t russian_roulette_terminations = 0;
    std::uint64_t invalid_pdf_terminations = 0;
    std::uint64_t invalid_contributions = 0;

    double samplesPerSecond() const {
        if (integration_time.count() <= 0.0) return 0.0;
        return static_cast<double>(sample_count) / integration_time.count();
    }

    double meanPathLength() const {
        if (sample_count == 0) return 0.0;
        return static_cast<double>(path_length_sum) /
               static_cast<double>(sample_count);
    }
};

/** Completed in-memory Film and render metrics. */
struct RenderResult {
    Film film;
    RenderStats stats;

    /** Convenience view of the Film beauty layer. */
    const Image& image() const { return film.beauty(); }
};

/**
 * Prepares one render job, allocates Film, and dispatches an Integrator.
 *
 * Renderer owns job configuration but contains no light-transport estimator.
 * Reuses the assembled Scene world and lights across render jobs.
 */
class Renderer {
public:
    explicit Renderer(RenderSettings settings = {},
                      RenderOptions options = {});

    const RenderSettings& settings() const { return settings_; }
    const RenderOptions& options() const { return options_; }

    /** Renders deterministic primary samples into a typed Film. */
    RenderResult render(
        const Scene& scene,
        RenderProgressCallback progress_callback = {}) const;

private:
    RenderSettings settings_;
    RenderOptions options_;
};

#endif  // NEARLIGHTER_RENDER_RENDERER_H
