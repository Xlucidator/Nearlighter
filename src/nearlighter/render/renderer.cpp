#include <nearlighter/render/renderer.h>

#include <nearlighter/render/render_context.h>
#include <nearlighter/scene/scene.h>

#include <stdexcept>
#include <utility>

Renderer::Renderer(RenderSettings settings, RenderOptions options)
    : settings_(settings), options_(options) {
    if (settings_.image_width <= 0 || settings_.image_height <= 0) {
        throw std::invalid_argument("Render image dimensions must be positive");
    }
    if (settings_.samples_per_pixel <= 0) {
        throw std::invalid_argument("Render samples per pixel must be positive");
    }
    if (settings_.max_depth <= 0) {
        throw std::invalid_argument(
            "Render maximum path depth must be positive");
    }
    if (options_.path.max_depth < 0 ||
        options_.path.russian_roulette_start_depth < 0) {
        throw std::invalid_argument(
            "Path integrator depth settings must not be negative");
    }
}

RenderResult Renderer::render(
    const Scene& scene,
    RenderProgressCallback progress_callback) const {
    using Clock = std::chrono::steady_clock;
    const auto preparation_start = Clock::now();

    if (options_.integrator == IntegratorKind::Path) {
        PathIntegrator::validateScene(scene);
    }

    const auto camera_start = Clock::now();
    const auto camera = scene.camera().prepare(settings_.image_width,
                                               settings_.image_height);
    const auto camera_end = Clock::now();
    const LightSampler light_sampler(scene.lights());
    const auto light_sampler_end = Clock::now();

    const auto film_start = Clock::now();
    Film film(settings_.image_width, settings_.image_height, options_.aovs);
    const auto film_end = Clock::now();
    std::unique_ptr<Integrator> integrator =
        createIntegrator(settings_, options_);
    const RenderContext context(scene, settings_, camera, light_sampler);
    const auto integration_start = Clock::now();

    const IntegrationResult integration = integrator->render(
        context, film, std::move(progress_callback));
    const auto end_time = Clock::now();

    RenderStats stats;
    stats.camera_preparation_time = camera_end - camera_start;
    stats.light_sampler_preparation_time = light_sampler_end - camera_end;
    stats.film_allocation_time = film_end - film_start;
    stats.preparation_time = integration_start - preparation_start;
    stats.integration_time =
        end_time - integration_start - integration.callback_time;

    const IntegratorCounters& counters = integration.counters;
    stats.sample_count = counters.sample_count;
    stats.camera_rays = counters.camera_rays;
    stats.continuation_rays = counters.continuation_rays;
    stats.shadow_rays = counters.shadow_rays;
    stats.surface_interactions = counters.surface_interactions;
    stats.medium_interactions = counters.medium_interactions;
    stats.path_length_sum = counters.path_length_sum;
    stats.max_depth_terminations = counters.max_depth_terminations;
    stats.russian_roulette_terminations =
        counters.russian_roulette_terminations;
    stats.invalid_pdf_terminations = counters.invalid_pdf_terminations;
    stats.invalid_contributions = counters.invalid_contributions;

    return RenderResult{std::move(film), stats};
}
