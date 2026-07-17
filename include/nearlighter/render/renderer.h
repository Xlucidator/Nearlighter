#ifndef NEARLIGHTER_RENDER_RENDERER_H
#define NEARLIGHTER_RENDER_RENDERER_H

#include <nearlighter/base/image.h>
#include <nearlighter/render/render_settings.h>

#include <chrono>
#include <cstdint>

class Ray;
class Sampler;
class Scene;
class Shape;

/** Records the core integration work performed by Renderer. */
struct RenderStats {
    std::chrono::duration<double> wall_time{};
    std::uint64_t sample_count = 0;
    double samples_per_second = 0.0;
};

/** Bundles a linear image with the statistics from the render that produced it. */
struct RenderResult {
    Image image;
    RenderStats stats;
};

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
     */
    RenderResult render(const Scene& scene) const;

private:
    Color trace(const Ray& ray, int depth, const Shape& world,
                const Shape& sampling_targets, const Color& background,
                Sampler& sampler) const;

    RenderSettings settings_;
};

#endif  // NEARLIGHTER_RENDER_RENDERER_H
