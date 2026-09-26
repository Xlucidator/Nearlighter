#ifndef NEARLIGHTER_LIGHT_LIGHT_SAMPLER_H
#define NEARLIGHTER_LIGHT_LIGHT_SAMPLER_H

#include <nearlighter/light/light.h>

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
 * Borrows a stable Scene light collection. It neither discovers emitters nor
 * owns lights. The membership index keeps emissive-hit PMF queries independent
 * of the number of lights.
 */
class LightSampler {
public:
    /** The non-null lights and their collection must outlive this sampler. */
    explicit LightSampler(const std::vector<std::unique_ptr<Light>>& lights);

    /** Selects with a uniform sample in [0, 1); returns no selection if empty. */
    std::optional<SelectedLight> select(float sample) const;

    /** Selection probability; zero for lights outside this collection. */
    float PMF(const Light& light) const;

private:
    const std::vector<std::unique_ptr<Light>>* lights_;
    std::unordered_set<const Light*> members_;
    float pmf_ = 0.0f;
};

#endif  // NEARLIGHTER_LIGHT_LIGHT_SAMPLER_H
