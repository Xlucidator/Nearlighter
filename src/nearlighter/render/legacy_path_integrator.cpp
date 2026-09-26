#include <nearlighter/render/integrator.h>

#include <nearlighter/base/interval.h>
#include <nearlighter/light/environment_light.h>
#include <nearlighter/material/material.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/scene.h>

#include <memory>
#include <stdexcept>
#include <utility>

IntegrationResult LegacyPathIntegrator::render(
    const RenderContext& context, Film& film,
    RenderProgressCallback progress_callback) const {
    using Clock = std::chrono::steady_clock;
    const auto integration_start = Clock::now();
    IntegrationResult result;
    const RenderSettings& settings = context.settings();
    const Scene& scene = context.scene();

    for (int y = 0; y < settings.image_height; ++y) {
        for (int x = 0; x < settings.image_width; ++x) {
            for (int sample_index = 0;
                 sample_index < settings.samples_per_pixel; ++sample_index) {
                Sampler sampler(derivePathSeed(
                    settings.seed, static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    static_cast<std::uint32_t>(sample_index)));
                const Ray ray = context.camera().generateRay(x, y, sampler);
                ++result.counters.camera_rays;
                std::uint64_t path_length = 0;
                const Color beauty = trace(
                    ray, max_depth_, scene.world(),
                    scene.samplingTargets(), scene.environment(),
                    sampler, result.counters, path_length);
                result.counters.path_length_sum += path_length;
                film.addBeautySample(x, y, beauty);
            }
        }

        if (progress_callback) {
            const auto callback_start = Clock::now();
            progress_callback(
                RenderProgress{
                    y + 1, settings.image_height,
                    callback_start - integration_start - result.callback_time},
                film);
            result.callback_time += Clock::now() - callback_start;
        }
    }

    result.counters.sample_count =
        static_cast<std::uint64_t>(settings.image_width) *
        static_cast<std::uint64_t>(settings.image_height) *
        static_cast<std::uint64_t>(settings.samples_per_pixel);
    return result;
}

Color LegacyPathIntegrator::trace(
    const Ray& ray, int depth, const Intersectable& world,
    const std::vector<std::shared_ptr<const Primitive>>& sampling_targets,
    const EnvironmentLight* environment, Sampler& sampler, IntegratorCounters& counters,
    std::uint64_t& path_length) const {
    if (depth <= 0) {
        ++counters.max_depth_terminations;
        return Color();
    }

    HitRecord record;
    if (!world.hit(ray, Interval(0.001f, infinity), record, sampler)) {
        return environment
                   ? environment->evaluateLi(unit_vector(ray.direction()))
                   : Color();
    }
    ++path_length;
    if (record.kind == InteractionKind::Surface) {
        ++counters.surface_interactions;
    } else {
        ++counters.medium_interactions;
    }
    if (!record.material) {
        throw std::runtime_error("Renderable shape has no material");
    }

    const Color emitted = record.material->emitted(
        ray, record, record.u, record.v, record.point);
    ScatterRecord scatter_record;
    if (!record.material->scatter(ray, record, scatter_record, sampler)) {
        return emitted;
    }

    if (scatter_record.should_skip) {
        ++counters.continuation_rays;
        return emitted + scatter_record.attenuation *
                             trace(scatter_record.skip_ray, depth - 1, world,
                                   sampling_targets, environment, sampler,
                                   counters, path_length);
    }
    if (!scatter_record.sampling_pdf) return emitted;

    std::shared_ptr<PDF> sample_pdf = scatter_record.sampling_pdf;
    if (!sampling_targets.empty()) {
        auto target_pdf = std::make_shared<SurfacePDF>(sampling_targets,
                                                       record.point);
        sample_pdf = std::make_shared<MixturePDF>(
            target_pdf, scatter_record.sampling_pdf);
    }

    const Ray scattered(record.point, sample_pdf->generate(sampler),
                        ray.time());
    const float pdf = sample_pdf->value(scattered.direction());
    if (pdf <= 0.0f) {
        ++counters.invalid_pdf_terminations;
        return emitted;
    }

    const float scattering_pdf =
        record.material->getScatterPDFValue(ray, record, scattered);
    ++counters.continuation_rays;
    const Color incoming = trace(scattered, depth - 1, world,
                                 sampling_targets, environment, sampler,
                                 counters, path_length);
    return emitted + scatter_record.attenuation * scattering_pdf * incoming /
                         pdf;
}
