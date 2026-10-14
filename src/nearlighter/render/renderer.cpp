#include <nearlighter/render/renderer.h>

#include <nearlighter/render/render_context.h>
#include <nearlighter/scene/scene.h>

#include <stdexcept>
#include <utility>

Renderer::Renderer(RenderSettings settings) : settings_(settings) {
    if (settings_.image_width <= 0 || settings_.image_height <= 0) {
        throw std::invalid_argument("Render image dimensions must be positive");
    }
    if (settings_.samples_per_pixel <= 0) {
        throw std::invalid_argument("Render samples per pixel must be positive");
    }
    // Validate algorithm settings even before a Scene is supplied.
    (void)PathIntegrator(settings_.path);
}

RenderResult Renderer::render(
    const Scene& scene,
    RenderProgressCallback progress_callback) const {
    using Clock = std::chrono::steady_clock;
    const auto preparation_start = Clock::now();

    PathIntegrator::validateScene(scene, settings_.path);

    const auto camera_start = Clock::now();
    const auto camera = scene.camera().prepare(settings_.image_width,
                                               settings_.image_height);
    const auto camera_end = Clock::now();
    const LightSampler light_sampler(scene.lights());
    const auto light_sampler_end = Clock::now();

    const auto film_start = Clock::now();
    Film film(settings_.image_width, settings_.image_height, settings_.aovs);
    const auto film_end = Clock::now();
    const PathIntegrator integrator(settings_.path);
    const RenderContext context(scene, settings_, camera, light_sampler);
    const auto integration_start = Clock::now();

    const IntegrationResult integration = integrator.render(
        context, film, std::move(progress_callback));
    const auto end_time = Clock::now();

    RenderStats stats;
    stats.camera_preparation_time = camera_end - camera_start;
    stats.light_sampler_preparation_time = light_sampler_end - camera_end;
    stats.film_allocation_time = film_end - film_start;
    stats.preparation_time = integration_start - preparation_start;
    stats.integration_time =
        end_time - integration_start - integration.callback_time;

    stats.counters = integration.counters;

    return RenderResult{std::move(film), stats};
}
