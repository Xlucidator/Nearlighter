#include <nearlighter/render/integrator.h>

#include <stdexcept>

std::unique_ptr<Integrator> createIntegrator(
    const RenderSettings& settings, const RenderOptions& options) {
    if (options.integrator == IntegratorKind::Legacy) {
        return std::make_unique<LegacyPathIntegrator>(settings.max_depth);
    }

    PathIntegratorOptions path_options = options.path;
    if (path_options.max_depth == 0) {
        path_options.max_depth = settings.max_depth;
    }
    if (path_options.max_depth <= 0) {
        throw std::invalid_argument(
            "Path integrator maximum depth must be positive");
    }
    if (path_options.russian_roulette_start_depth < 0) {
        throw std::invalid_argument(
            "Russian roulette start depth must not be negative");
    }
    return std::make_unique<PathIntegrator>(path_options);
}
