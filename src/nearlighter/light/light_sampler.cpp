#include <nearlighter/light/light_sampler.h>

#include <algorithm>
#include <stdexcept>

LightSampler::LightSampler(const std::vector<std::unique_ptr<Light>>& lights)
    : lights_(&lights) {
    if (lights.empty()) return;
    pmf_ = 1.0f / static_cast<float>(lights.size());
    members_.reserve(lights.size());
    for (const auto& light : lights) {
        if (!light) throw std::invalid_argument("LightSampler requires non-null lights");
        members_.insert(light.get());
    }
}

std::optional<SelectedLight> LightSampler::select(float sample) const {
    if (lights_->empty()) return std::nullopt;
    const std::size_t index = std::min(
        static_cast<std::size_t>(sample * lights_->size()), lights_->size() - 1);
    return SelectedLight{(*lights_)[index].get(), pmf_};
}

float LightSampler::PMF(const Light& light) const {
    return members_.count(&light) ? pmf_ : 0.0f;
}
