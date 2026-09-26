#include <nearlighter/render/integrator.h>

#include <nearlighter/base/interval.h>
#include <nearlighter/light/area_light.h>
#include <nearlighter/light/environment_light.h>
#include <nearlighter/material/material.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/scene.h>
#include <nearlighter/scene/surface_interaction.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace {

bool finiteColor(const Color& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) &&
           std::isfinite(value.z());
}

float maximumComponent(const Color& value) {
    return std::max({value.x(), value.y(), value.z()});
}

float powerHeuristic(float first_pdf, float second_pdf) {
    const float first_squared = first_pdf * first_pdf;
    const float second_squared = second_pdf * second_pdf;
    const float denominator = first_squared + second_squared;
    return denominator > 0.0f ? first_squared / denominator : 0.0f;
}

void addContribution(FilmSample& sample, const Color& contribution,
                     int scattering_events,
                     IntegratorCounters& counters) {
    if (!finiteColor(contribution)) {
        ++counters.invalid_contributions;
        return;
    }

    sample.beauty += contribution;
    if (scattering_events == 0) {
        sample.emission += contribution;
    } else if (scattering_events == 1) {
        sample.direct += contribution;
    } else {
        sample.indirect += contribution;
    }
}

bool validSampledEvent(const SurfaceInteraction& interaction,
                       const BSDFSample& sample) {
    const float outgoing_side = dot(interaction.outgoing(),
                                    interaction.geometricNormal());
    const float incoming_side = dot(sample.incoming,
                                    interaction.geometricNormal());
    if (hasAnyFlag(sample.flags, BxDFFlags::Reflection)) {
        return outgoing_side * incoming_side > 0.0f;
    }
    if (hasAnyFlag(sample.flags, BxDFFlags::Transmission)) {
        return outgoing_side * incoming_side < 0.0f;
    }
    return false;
}

bool visible(const SurfaceInteraction& reference, const LightSample& light,
             float time, const Intersectable& world, Sampler& sampler,
             IntegratorCounters& counters) {
    const Ray shadow_ray = reference.spawnRay(light.incoming, time);
    ++counters.shadow_rays;
    HitRecord blocker;
    if (std::isinf(light.distance)) {
        return !world.hit(shadow_ray, Interval(0.0f, infinity), blocker,
                          sampler);
    }

    const float maximum_t = std::nextafter(light.distance, 0.0f);
    if (!(maximum_t > 0.0f)) return false;
    return !world.hit(shadow_ray, Interval(0.0f, maximum_t), blocker,
                      sampler);
}

float emissionMISWeight(DirectLightingMode mode, bool previous_specular,
                        int scattering_events, float bsdf_pdf,
                        float light_pdf) {
    if (scattering_events == 0 || previous_specular ||
        mode == DirectLightingMode::BSDFOnly) {
        return 1.0f;
    }
    if (mode == DirectLightingMode::LightOnly) return 0.0f;
    return powerHeuristic(bsdf_pdf, light_pdf);
}

}  // namespace

void PathIntegrator::validateScene(const Scene& scene) {
    std::ostringstream issues;
    if (scene.containsMedia()) issues << "\n- participating media are not supported";
    if (!scene.inspectionComplete()) {
        issues << "\n- custom Intersectable content cannot be inspected";
    }
    std::size_t unsupported_materials = 0;
    for (const Material* material : scene.materials()) {
        if (!material->supportsPathIntegrator()) ++unsupported_materials;
    }
    if (unsupported_materials) {
        issues << "\n- " << unsupported_materials << " unsupported surface material(s)";
    }
    const std::size_t unsampleable_emitters = scene.unsampleableEmitterCount();
    if (unsampleable_emitters) {
        issues << "\n- " << unsampleable_emitters
               << " emissive surface(s) have no direction-sampling PDF";
    }
    if (!issues.str().empty()) {
        throw std::invalid_argument(
            "Scene is incompatible with the path integrator:" + issues.str());
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
 * The loop stores the previous BSDF strategy so an emissive hit can reconstruct
 * the same two PDFs used by NEE. Delta events bypass the continuous heuristic.
 * Contributions are classified by the number of completed scattering events,
 * keeping `beauty = emission + direct + indirect` true per primary sample.
 */
FilmSample PathIntegrator::trace(
    const Ray& camera_ray, const RenderContext& context, Sampler& sampler,
    IntegratorCounters& counters, std::uint64_t& path_length) const {
    const Scene& scene = context.scene();
    const LightSampler& light_sampler = context.lightSampler();
    FilmSample result{};
    Ray ray = camera_ray;
    Color beta(1.0f, 1.0f, 1.0f);
    int scattering_events = 0;
    bool previous_specular = true;
    float previous_bsdf_pdf = 0.0f;
    float eta_scale = 1.0f;
    std::optional<SurfaceInteraction> previous_interaction;

    while (true) {
        /* ----- Closest interaction and escaped radiance ----- */
        HitRecord record;
        if (!scene.world().hit(ray, Interval(0.0f, infinity), record,
                               sampler)) {
            const auto* environment = scene.environment();
            const Vec3f direction = unit_vector(ray.direction());
            const Color radiance = environment
                                       ? environment->evaluateLi(direction)
                                       : Color();
            const float light_pdf = previous_interaction && environment
                ? light_sampler.PMF(*environment) *
                      environment->PDFLi(*previous_interaction, direction)
                : 0.0f;
            const float weight = emissionMISWeight(
                options_.direct_lighting, previous_specular,
                scattering_events, previous_bsdf_pdf, light_pdf);
            addContribution(result, beta * radiance * weight,
                            scattering_events, counters);
            break;
        }

        ++path_length;
        if (record.kind == InteractionKind::Medium) {
            ++counters.medium_interactions;
            throw std::runtime_error(
                "Path integrator encountered an unsupported medium event");
        }
        ++counters.surface_interactions;
        const SurfaceInteraction interaction(ray, record);

        /* ----- Emissive-hit strategy ----- */
        const Color emitted = interaction.material().evaluateEmission(
            interaction, interaction.outgoing());
        if (!emitted.near_zero()) {
            const auto* light = scene.findAreaLights(interaction.primitive());
            const float light_pdf = previous_interaction && light
                ? light_sampler.PMF(*light) *
                      light->PDFLi(*previous_interaction, unit_vector(ray.direction()))
                : 0.0f;
            const float weight = emissionMISWeight(
                options_.direct_lighting, previous_specular,
                scattering_events, previous_bsdf_pdf, light_pdf);
            addContribution(result, beta * emitted * weight,
                            scattering_events, counters);
        }

        const auto bsdf = interaction.material().computeBSDF(
            interaction, TransportMode::Radiance);
        if (!bsdf || bsdf->empty()) break;
        if (scattering_events >= options_.max_depth) {
            ++counters.max_depth_terminations;
            break;
        }

        /* ----- One-sample next-event estimation ----- */
        if (options_.direct_lighting != DirectLightingMode::BSDFOnly &&
            !scene.lights().empty()) {
            const auto selected = light_sampler.select(sampler.next1D());
            const auto light = selected
                ? selected->light->sampleLi(interaction, ray.time(), sampler)
                : std::nullopt;
            const float light_pdf = light ? selected->pmf * light->pdf : 0.0f;
            if (light && light_pdf > 0.0f &&
                finiteColor(light->radiance)) {
                const Color value = bsdf->evaluate(
                    interaction.outgoing(), light->incoming,
                    TransportMode::Radiance);
                const float bsdf_pdf = bsdf->PDF(
                    interaction.outgoing(), light->incoming,
                    TransportMode::Radiance);
                const float cosine = std::fabs(dot(
                    interaction.shadingNormal(), light->incoming));
                const bool reflection =
                    dot(interaction.outgoing(),
                        interaction.geometricNormal()) *
                        dot(light->incoming,
                            interaction.geometricNormal()) >
                    0.0f;
                if (reflection && !value.near_zero() && cosine > 0.0f &&
                    visible(interaction, *light, ray.time(), scene.world(),
                            sampler, counters)) {
                    const float weight =
                        options_.direct_lighting ==
                                DirectLightingMode::MIS
                            ? powerHeuristic(light_pdf, bsdf_pdf)
                            : 1.0f;
                    addContribution(
                        result,
                        beta * value * light->radiance *
                            (cosine * weight / light_pdf),
                        scattering_events + 1, counters);
                }
            }
        }

        /* ----- BSDF continuation ----- */
        const auto bsdf_sample = bsdf->sample(
            interaction.outgoing(), sampler.next1D(), sampler.next2D(),
            TransportMode::Radiance);
        if (!bsdf_sample || !(bsdf_sample->pdf > 0.0f) ||
            !std::isfinite(bsdf_sample->pdf) ||
            !validSampledEvent(interaction, *bsdf_sample)) {
            ++counters.invalid_pdf_terminations;
            break;
        }

        const float cosine = std::fabs(dot(interaction.shadingNormal(),
                                           bsdf_sample->incoming));
        beta = beta * bsdf_sample->value * (cosine / bsdf_sample->pdf);
        if (!finiteColor(beta) || maximumComponent(beta) < 0.0f) {
            ++counters.invalid_contributions;
            break;
        }

        ++scattering_events;
        previous_interaction = interaction;
        previous_bsdf_pdf = bsdf_sample->pdf;
        previous_specular =
            hasAnyFlag(bsdf_sample->flags, BxDFFlags::Specular);
        if (hasAnyFlag(bsdf_sample->flags, BxDFFlags::Transmission)) {
            eta_scale *= bsdf_sample->eta * bsdf_sample->eta;
        }
        ray = interaction.spawnRay(bsdf_sample->incoming, ray.time());
        ++counters.continuation_rays;

        /* ----- Optional unbiased path termination ----- */
        if (options_.russian_roulette_start_depth > 0 &&
            scattering_events >=
                options_.russian_roulette_start_depth) {
            const float survival_probability = std::clamp(
                maximumComponent(beta * eta_scale), 0.0f, 0.95f);
            if (!(survival_probability > 0.0f) ||
                sampler.next1D() >= survival_probability) {
                ++counters.russian_roulette_terminations;
                break;
            }
            beta /= survival_probability;
        }
    }

    return result;
}
