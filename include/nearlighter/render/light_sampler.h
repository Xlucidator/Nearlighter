#ifndef NEARLIGHTER_RENDER_LIGHT_SAMPLER_H
#define NEARLIGHTER_RENDER_LIGHT_SAMPLER_H

#include <nearlighter/scene/light/light.h>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

/** One light and its discrete selection probability. */
struct SelectedLight {
    const Light* light;
    float pmf;
};

/**
 * Uniform Light Selection
 *
 * Collection and strategy:
 * - Borrows a stable Scene light collection.
 * - Selects lights without discovering or owning them.
 * - Uses a membership index for constant-time emissive-hit PMF queries.
 */
class LightSampler {
public:
    /** The non-null lights and their collection must outlive this sampler. */
    explicit LightSampler(const std::vector<std::unique_ptr<Light>>& lights);

    /** Selects with a uniform sample in [0, 1); returns no selection if empty. */
    std::optional<SelectedLight> select(float sample) const;

    /**
     * Discrete Light Selection Probability
     *
     * Returns zero for lights outside this collection.
     */
    float PMF(const Light& light) const;

private:
    const std::vector<std::unique_ptr<Light>>* lights_;
    std::unordered_set<const Light*> members_;
    float pmf_ = 0.0f;
};

#endif  // NEARLIGHTER_RENDER_LIGHT_SAMPLER_H
