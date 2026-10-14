#include <nearlighter/render/integrator.h>
#include <nearlighter/render/render_context.h>

#include <nearlighter/scene/light/area_light.h>
#include <nearlighter/scene/light/environment_light.h>
#include <nearlighter/material/material.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/scene.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace {

bool finiteColor(const Color& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) && std::isfinite(value.z());
}

float maximumComponent(const Color& value) {
    return std::max({value.x(), value.y(), value.z()});
}

float powerHeuristic(float first_pdf, float second_pdf) {
    // Double intermediates avoid overflow for concentrated glossy proposals.
    const double first_squared = double(first_pdf) * first_pdf;
    const double second_squared = double(second_pdf) * second_pdf;
    const double denominator = first_squared + second_squared;
    return denominator > 0.0 ? float(first_squared / denominator) : 0.0f;
}

bool usesNEE(PathSamplingMode mode) {
    return mode == PathSamplingMode::LightOnly || mode == PathSamplingMode::MIS;
}

void addContribution(FilmSample& sample, const Color& contribution,
                     int scattering_events, IntegratorCounters& counters) {
    if (!finiteColor(contribution)) {
        ++counters.invalid_contributions;
        return;
    }
    sample.beauty += contribution;
    if (scattering_events == 0) sample.emission += contribution;
    else if (scattering_events == 1) sample.direct += contribution;
    else sample.indirect += contribution;
}

bool validSampledEvent(const SurfaceRecord& record, const ScatteringSample& sample) {
    const float sides = dot(record.outgoing, record.geometric_normal) *
                        dot(sample.incoming, record.geometric_normal);
    if (hasAnyFlag(sample.flags, BxDFFlags::Reflection)) return sides > 0.0f;
    if (hasAnyFlag(sample.flags, BxDFFlags::Transmission)) return sides < 0.0f;
    return false;
}

bool visible(const HitRecord& reference, const LightSample& light,
             float time, const Intersectable& world, Sampler& sampler,
             IntegratorCounters& counters) {
    const Ray shadow_ray = reference.spawnRay(light.incoming, time);
    ++counters.shadow_rays;
    HitRecord blocker;
    const float maximum_t = std::isinf(light.distance)
        ? infinity : std::nextafter(light.distance, 0.0f);
    return maximum_t > 0.0f &&
        !world.hit(shadow_ray, Interval(0.0f, maximum_t), blocker, sampler);
}

float emissionMISWeight(PathSamplingMode mode, bool previous_specular,
                        int scattering_events, float scattering_pdf, float light_pdf) {
    if (scattering_events == 0 || previous_specular || !usesNEE(mode)) return 1.0f;
    if (mode == PathSamplingMode::LightOnly) return 0.0f;
    return powerHeuristic(scattering_pdf, light_pdf);
}

/** Continuation Weight Already Divided by the Actual Direction Proposal */
struct PathStep {
    Vec3f incoming;
    Color weight;
    float pdf;
    float eta = 1.0f;
    bool specular = false;
};

/** Shared Proposal Selection; Surface Projection Belongs to the Path Integral */
template<class Response>
std::optional<PathStep> sampleContinuation(
    const Vec3f& outgoing, const SurfaceRecord* surface,
    const Response& response, const SurfacePDF& targets,
    bool mixture, Sampler& sampler, IntegratorCounters& counters) {
    /* ----- Direction Proposal ----- */
    const float response_probability = mixture ? 0.5f : 1.0f;
    std::optional<ScatteringSample> sample;
    if (mixture && sampler.next1D() >= response_probability) {
        const Vec3f direction = targets.generate(sampler);
        if (direction.near_zero()) return std::nullopt;
        const Vec3f incoming = unit_vector(direction);
        auto event = BxDFFlags::None;
        if (surface) {
            const float sides = dot(outgoing, surface->geometric_normal) *
                                dot(incoming, surface->geometric_normal);
            event = sides > 0.0f ? BxDFFlags::Reflection : BxDFFlags::Transmission;
        }
        sample = ScatteringSample{response.evaluate(outgoing, incoming), incoming,
                                  response.PDF(outgoing, incoming), event};
    } else {
        // Preserve the discrete probability and coefficient of a sampled delta event.
        sample = response.sample(outgoing, sampler);
    }
    if (!sample || (surface && !validSampledEvent(*surface, *sample))) return std::nullopt;

    /* ----- Proposal Measure and Path Weight ----- */
    const bool specular = hasAnyFlag(sample->flags, BxDFFlags::Specular);
    // The sample already contains the response density; avoid repeating its model query.
    const float pdf = mixture && !specular
        ? response_probability * sample->pdf +
          (1.0f - response_probability) * targets.value(sample->incoming)
        : response_probability * sample->pdf;
    // A delta selected from a mixed BSDF still includes the outer strategy PMF.
    if (!(pdf > 0.0f) || !std::isfinite(pdf)) {
        ++counters.invalid_pdf_terminations;
        return std::nullopt;
    }
    const float projection = surface ? std::fabs(dot(surface->normal, sample->incoming)) : 1.0f;
    return PathStep{sample->incoming, sample->value * (projection / pdf),
                    pdf, sample->eta, specular};
}

}  // namespace

PathIntegrator::PathIntegrator(PathIntegratorOptions options) : options_(options) {
    if (options.max_depth <= 0 || options.russian_roulette_start_depth < 0) {
        throw std::invalid_argument("Path depth must be positive and roulette depth nonnegative");
    }
}

void PathIntegrator::validateScene(const Scene& scene, const PathIntegratorOptions& options) {
    if (!usesNEE(options.sampling)) return;
    std::ostringstream issues;
    if (scene.containsMedia()) {
        issues << "\n- media require scattering or mixture sampling; volume NEE is not implemented";
    }
    if (!scene.inspectionComplete()) issues << "\n- custom Intersectable content cannot be inspected for NEE";
    if (scene.unsampleableEmitterCount()) {
        issues << "\n- emissive surfaces without direction PDFs require scattering or mixture sampling";
    }
    if (!issues.str().empty()) {
        throw std::invalid_argument("Scene is incompatible with this sampling mode:" + issues.str());
    }
}

IntegrationResult PathIntegrator::render(
    const RenderContext& context, Film& film,
    RenderProgressCallback progress_callback) const {
    using Clock = std::chrono::steady_clock;
    const auto integration_start = Clock::now();
    IntegrationResult result;
    const RenderSettings& settings = context.settings();

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
                film.addSample(x, y,
                               trace(ray, context, sampler, result.counters,
                                     path_length));
                result.counters.path_length_sum += path_length;
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

/**
 * @par Implementation
 * Previous-event state reconstructs the competing light PDF at emissive hits.
 * Delta events bypass continuous MIS; Mixture never adds a separate NEE estimate.
 * Completed scattering counts preserve beauty = emission + direct + indirect.
 */
FilmSample PathIntegrator::trace(
    const Ray& camera_ray, const RenderContext& context, Sampler& sampler,
    IntegratorCounters& counters, std::uint64_t& path_length) const {
    const Scene& scene = context.scene();
    const LightSampler& light_sampler = context.lightSampler();
    FilmSample result{};
    Ray ray = camera_ray;
    Color beta(1, 1, 1);
    int scattering_events = 0;
    bool previous_specular = true;
    float previous_pdf = 0.0f;
    float eta_scale = 1.0f;
    std::optional<HitRecord> previous_record;
    const bool mixture = options_.sampling == PathSamplingMode::Mixture &&
                         !scene.samplingTargets().empty();

    while (true) {
        /* ----- Closest Event and Escaped Radiance ----- */
        HitRecord record;
        const bool need_mis_pdf = options_.sampling == PathSamplingMode::MIS &&
                                  previous_record && !previous_specular;
        if (!scene.world().hit(ray, Interval(0.0f, infinity), record, sampler)) {
            const auto* environment = scene.environment();
            const Vec3f direction = unit_vector(ray.direction());
            const Color radiance = environment ? environment->evaluateLi(direction) : Color();
            const float light_pdf = need_mis_pdf && environment
                ? light_sampler.PMF(*environment) * environment->PDFLi(*previous_record, direction)
                : 0.0f;
            const float weight = emissionMISWeight(options_.sampling, previous_specular,
                                                   scattering_events, previous_pdf, light_pdf);
            addContribution(result, beta * radiance * weight, scattering_events, counters);
            break;
        }
        ++path_length;
        const auto* surface = record.isSurface() ? &record.surface() : nullptr;
        if (surface) ++counters.surface_interactions;
        else ++counters.medium_interactions;
        const Material& material = record.material();
        const Vec3f& outgoing = record.outgoing();

        /* ----- Event Emission ----- */
        if (!surface && material.isEmissive()) {
            // Volume sources require distance integration, not a surface-hit radiance term.
            throw std::invalid_argument("Volume emission is not implemented by PathIntegrator");
        }
        if (surface) {
            const Color emitted = material.emission(record, outgoing);
            if (!emitted.near_zero()) {
                const auto* light = need_mis_pdf ? scene.findAreaLights(record.primitive()) : nullptr;
                const float light_pdf = light
                    ? light_sampler.PMF(*light) * light->PDFLi(*previous_record, unit_vector(ray.direction()))
                    : 0.0f;
                const float weight = emissionMISWeight(options_.sampling, previous_specular,
                                                       scattering_events, previous_pdf, light_pdf);
                addContribution(result, beta * emitted * weight, scattering_events, counters);
            }
        }
        if (scattering_events >= options_.max_depth) {
            ++counters.max_depth_terminations;
            break;
        }

        /* ----- Final-Event Scattering Evaluation ----- */
        const ScatteringResponse response = material.scattering(record);
        if (std::holds_alternative<std::monostate>(response)) break;
        const BSDF* bsdf = surface ? &std::get<BSDF>(response) : nullptr;
        if (bsdf && bsdf->empty()) break;

        /* ----- Surface Next-Event Estimation ----- */
        if (surface && usesNEE(options_.sampling) && !scene.lights().empty()) {
            const auto selected = light_sampler.select(sampler.next1D());
            const auto light = selected
                ? selected->light->sampleLi(record, ray.time(), sampler) : std::nullopt;
            const float light_pdf = light ? selected->pmf * light->pdf : 0.0f;
            if (light && light_pdf > 0.0f && finiteColor(light->radiance)) {
                const Color value = bsdf->evaluate(surface->outgoing, light->incoming);
                const float scattering_pdf = bsdf->PDF(surface->outgoing, light->incoming);
                const float cosine = std::fabs(dot(surface->normal, light->incoming));
                const bool reflection = dot(surface->outgoing, surface->geometric_normal) *
                                        dot(light->incoming, surface->geometric_normal) > 0.0f;
                if (reflection && !value.near_zero() && cosine > 0.0f &&
                    visible(record, *light, ray.time(), scene.world(), sampler, counters)) {
                    const float weight = options_.sampling == PathSamplingMode::MIS
                        ? powerHeuristic(light_pdf, scattering_pdf) : 1.0f;
                    addContribution(result, beta * value * light->radiance * (cosine * weight / light_pdf),
                                    scattering_events + 1, counters);
                }
            }
        }

        /* ----- Surface or Volume Continuation ----- */
        const SurfacePDF targets(scene.samplingTargets(), record.point());
        // Pure delta responses bypass continuous target proposals.
        const bool use_mixture = mixture && (!surface ||
            hasAnyFlag(bsdf->flags(), BxDFFlags::Diffuse | BxDFFlags::Glossy));
        // Visit once per event so the shared algorithm keeps concrete response types.
        const auto step = std::visit([&](const auto& evaluated) -> std::optional<PathStep> {
            using Response = std::decay_t<decltype(evaluated)>;
            if constexpr (std::is_same_v<Response, std::monostate>) {
                return std::nullopt;
            } else {
                return sampleContinuation(outgoing, surface, evaluated, targets,
                                          use_mixture, sampler, counters);
            }
        }, response);
        // A rejected proposal contributes zero; it is not an invalid numerical PDF.
        if (!step) break;
        beta = beta * step->weight;
        if (!finiteColor(beta) || maximumComponent(beta) < 0.0f) {
            ++counters.invalid_contributions;
            break;
        }
        if (maximumComponent(beta) == 0.0f) break;
        ++scattering_events;
        previous_record = record;
        previous_pdf = step->pdf;
        previous_specular = step->specular;
        eta_scale *= step->eta * step->eta;
        ray = record.spawnRay(step->incoming, ray.time());
        ++counters.continuation_rays;

        /* ----- Optional Unbiased Path Termination ----- */
        if (options_.russian_roulette_start_depth > 0 &&
            scattering_events >= options_.russian_roulette_start_depth) {
            const float survival = std::clamp(maximumComponent(beta * eta_scale), 0.0f, 0.95f);
            if (!(survival > 0.0f) || sampler.next1D() >= survival) {
                ++counters.russian_roulette_terminations;
                break;
            }
            beta /= survival;
        }
    }
    return result;
}
