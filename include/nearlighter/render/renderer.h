#ifndef NEARLIGHTER_RENDER_RENDERER_H
#define NEARLIGHTER_RENDER_RENDERER_H

#include <nearlighter/base/image.h>
#include <nearlighter/render/render_settings.h>

#include <chrono>
#include <cstdint>
#include <functional>

class Ray;
class Sampler;
class Scene;
class Shape;

// ============================================================================
// Render Lifecycle Data
//
// Renderer::render()
//   +-- emits RenderProgress while integrating
//   `-- returns RenderResult
//         +-- Image
//         `-- RenderStats
// ============================================================================

/** Final integration metrics */
struct RenderStats {
    std::chrono::duration<double> integration_time{};
    std::uint64_t sample_count = 0;

    /** Primary sample throughput */
    double samplesPerSecond() const {
        if (integration_time.count() <= 0.0) return 0.0;
        return static_cast<double>(sample_count) / integration_time.count();
    }
};

/** Completed image and integration metrics */
struct RenderResult {
    Image image;
    RenderStats stats;
};

/** Row-level integration state */
struct RenderProgress {
    int completed_rows = 0;
    int total_rows = 0;
    std::chrono::duration<double> integration_time{};
};

/**
 * Receives row-level progress and a read-only view of the partial image.
 *
 * Rows before completed_rows contain final values for this render. The Image
 * reference is valid only for the duration of the callback invocation.
 */
using RenderProgressCallback =
    std::function<void(const RenderProgress&, const Image&)>;

/** Integrates a Scene into a linear RGB Image without performing file I/O. */
class Renderer {
public:
    explicit Renderer(RenderSettings settings = {});

    /** Returns the effective settings owned by this Renderer. */
    const RenderSettings& settings() const { return settings_; }

    /**
     * Renders a Scene using deterministic per-pixel sample sequences.
     *
     * BVH construction and camera preparation occur before the reported core
     * integration time, matching the previous command-line timing boundary.
     * When supplied, progress_callback runs once after every completed row.
     * Its execution time is excluded from the returned render statistics.
     * Exceptions raised by the callback propagate to the caller.
     */
    RenderResult render(
        const Scene& scene,
        RenderProgressCallback progress_callback = {}
    ) const;

private:
    Color trace(const Ray& ray, int depth, const Shape& world,
                const Shape& sampling_targets, const Color& background,
                Sampler& sampler) const;

    RenderSettings settings_;
};

#endif  // NEARLIGHTER_RENDER_RENDERER_H
