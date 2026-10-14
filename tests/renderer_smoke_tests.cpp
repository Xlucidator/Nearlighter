#include "test_support.h"
#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/material/medium/isotropic.h>
#include <nearlighter/material/surface/metal.h>
#include <nearlighter/shape/quad.h>

#include <nearlighter/material/surface/emissive.h>
#include <nearlighter/material/surface/lambertian.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/scene/scene.h>
#include <nearlighter/shape/sphere.h>

#include <cmath>
#include <cstdint>
#include <memory>

namespace {

Scene makeScene() {
    Camera camera;
    camera.vertical_fov = 45.0f;
    camera.position = Point3f(0.0f, 0.0f, 0.0f);
    camera.look_at = Point3f(0.0f, 0.0f, -1.0f);
    camera.focus_distance = 1.0f;

    RenderSettings settings;
    settings.image_width = 13;
    settings.image_height = 11;
    settings.samples_per_pixel = 3;
    settings.path.max_depth = 4;
    settings.seed = 123;

    LinearAggregate world;
    world.add(std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, -1.0f), 0.5f),
        std::make_shared<Emissive>(Color(2.0f, 1.0f, 0.5f))));
    return Scene("Renderer smoke test", camera, settings,
                 Color(0.05f, 0.1f, 0.2f), std::move(world));
}

Scene makePathScene() {
    Camera camera;
    camera.vertical_fov = 45.0f;
    camera.position = Point3f(0.0f, 0.0f, 0.0f);
    camera.look_at = Point3f(0.0f, 0.0f, -1.0f);
    camera.focus_distance = 1.0f;

    RenderSettings settings;
    settings.image_width = 13;
    settings.image_height = 11;
    settings.samples_per_pixel = 4;
    settings.path.max_depth = 4;
    settings.seed = 321;

    LinearAggregate world;
    world.add(std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, -1.0f), 0.5f),
        std::make_shared<Lambertian>(Color(0.4f, 0.5f, 0.6f))));
    return Scene("Path smoke test", camera, settings,
                 Color(0.2f, 0.25f, 0.3f), std::move(world));
}

bool imagesEqual(const Image& lhs, const Image& rhs) {
    if (lhs.width() != rhs.width() || lhs.height() != rhs.height()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.pixels().size(); ++index) {
        const Color& a = lhs.pixels()[index];
        const Color& b = rhs.pixels()[index];
        if (a.x() != b.x() || a.y() != b.y() || a.z() != b.z()) {
            return false;
        }
    }
    return true;
}

void testRenderer(nearlighter::test::Context& context) {
    const Scene scene = makeScene();
    RenderSettings settings = scene.defaultRenderSettings();
    const Renderer renderer(settings);
    int progress_updates = 0;
    bool progress_contract_valid = true;
    double previous_integration_time = 0.0;
    const RenderResult first = renderer.render(
        scene,
        [&](const RenderProgress& progress, const Film& partial_film) {
            const Image& partial_image = partial_film.beauty();
            ++progress_updates;
            progress_contract_valid &=
                progress.completed_rows == progress_updates &&
                progress.total_rows == settings.image_height &&
                partial_image.width() == settings.image_width &&
                partial_image.height() == settings.image_height &&
                progress.integration_time.count() >=
                    previous_integration_time;
            previous_integration_time = progress.integration_time.count();
        }
    );
    const RenderResult second = renderer.render(scene);

    context.expectTrue(first.image().width() == settings.image_width,
                       "rendered image width should match RenderSettings");
    context.expectTrue(first.image().height() == settings.image_height,
                       "rendered image height should match RenderSettings");
    context.expectTrue(imagesEqual(first.image(), second.image()),
                       "equal scene settings and seed should reproduce every pixel");
    context.expectTrue(progress_updates == settings.image_height,
                       "Renderer should report each completed image row");
    context.expectTrue(progress_contract_valid,
                       "render progress should be ordered and match the output image");

    bool non_black = false;
    bool all_finite = true;
    for (const Color& pixel : first.image().pixels()) {
        non_black |= pixel.x() != 0.0f || pixel.y() != 0.0f ||
                     pixel.z() != 0.0f;
        all_finite &= std::isfinite(pixel.x()) && std::isfinite(pixel.y()) &&
                      std::isfinite(pixel.z());
    }
    context.expectTrue(non_black, "smoke render should not be entirely black");
    context.expectTrue(all_finite,
                       "smoke render should contain only finite pixel values");

    const std::uint64_t expected_samples =
        static_cast<std::uint64_t>(settings.image_width) *
        static_cast<std::uint64_t>(settings.image_height) *
        static_cast<std::uint64_t>(settings.samples_per_pixel);
    context.expectTrue(first.stats.counters.sample_count == expected_samples,
                       "render statistics should report every primary sample");
    context.expectTrue(first.stats.integration_time.count() >= 0.0,
                       "render integration time should not be negative");

    RenderSettings changed_settings = settings;
    ++changed_settings.seed;
    const RenderResult changed = Renderer(changed_settings).render(scene);
    context.expectFalse(imagesEqual(first.image(), changed.image()),
                        "changing the render seed should change sampled pixels");
}

void testPathIntegrator(nearlighter::test::Context& context) {
    const Scene scene = makePathScene();
    RenderSettings settings = scene.defaultRenderSettings();

    settings.path.sampling = PathSamplingMode::MIS;
    settings.aovs = FilmAOV::All;

    const RenderResult first = Renderer(settings).render(scene);
    const RenderResult second = Renderer(settings).render(scene);
    context.expectTrue(imagesEqual(first.image(), second.image()),
                       "path integrator should preserve deterministic sampling");
    context.expectTrue(first.stats.counters.shadow_rays > 0,
                       "MIS path render should cast explicit shadow rays");
    context.expectTrue(first.stats.counters.continuation_rays > 0,
                       "path render should sample BSDF continuation rays");
    context.expectTrue(first.stats.counters.russian_roulette_terminations == 0,
                       "Russian roulette should be disabled by default");
    context.expectTrue(first.stats.counters.invalid_pdf_terminations == 0 &&
                           first.stats.counters.invalid_contributions == 0,
                       "supported diffuse path scene should avoid invalid events");

    RenderSettings beauty_settings = settings;
    beauty_settings.aovs = FilmAOV::Beauty;
    const RenderResult beauty = Renderer(beauty_settings).render(scene);
    context.expectTrue(beauty.film.enabledAOVs() == FilmAOV::Beauty,
                       "RenderSettings selects the allocated output layers");
    context.expectTrue(imagesEqual(first.image(), beauty.image()),
                       "output layers do not change deterministic path sampling");

    bool direct_contribution_found = false;
    bool decomposition_valid = true;
    for (int y = 0; y < settings.image_height; ++y) {
        for (int x = 0; x < settings.image_width; ++x) {
            const Color classified = first.film.emission().at(x, y) +
                                     first.film.direct().at(x, y) +
                                     first.film.indirect().at(x, y);
            const Color delta = first.film.beauty().at(x, y) - classified;
            decomposition_valid &= std::fabs(delta.x()) < 1e-5f &&
                                   std::fabs(delta.y()) < 1e-5f &&
                                   std::fabs(delta.z()) < 1e-5f;
            direct_contribution_found |=
                !first.film.direct().at(x, y).near_zero();
            decomposition_valid &=
                first.film.sampleCountAt(x, y) ==
                static_cast<std::uint64_t>(settings.samples_per_pixel);
        }
    }
    context.expectTrue(decomposition_valid,
                       "path Film should preserve its AOV identity and counts");
    context.expectTrue(direct_contribution_found,
                       "environment NEE should populate the direct AOV");

    RenderSettings bsdf_only_settings = settings;
    bsdf_only_settings.path.sampling = PathSamplingMode::ScatteringOnly;
    const RenderResult bsdf_only =
        Renderer(bsdf_only_settings).render(scene);
    context.expectTrue(bsdf_only.stats.counters.shadow_rays == 0,
                       "BSDF-only mode should disable next-event estimation");

    RenderSettings light_only_settings = settings;
    light_only_settings.path.sampling = PathSamplingMode::LightOnly;
    const RenderResult light_only =
        Renderer(light_only_settings).render(scene);
    context.expectTrue(light_only.stats.counters.shadow_rays > 0,
                       "light-only mode should retain next-event estimation");

    RenderSettings roulette_settings = settings;
    roulette_settings.path.russian_roulette_start_depth = 1;
    const RenderResult roulette =
        Renderer(roulette_settings).render(scene);
    context.expectTrue(roulette.stats.counters.russian_roulette_terminations > 0,
                       "enabled Russian roulette should terminate some paths");
}

}  // namespace


namespace {
/** Analytic Sum of a Diffuse Lobe and a Discrete Mirror Lobe */
class MixedSurface final : public Material {
public:
    MaterialKind kind() const override { return MaterialKind::Surface; }
    ScatteringResponse scattering(
        const HitRecord& record, [[maybe_unused]] TransportMode mode) const override {
        BSDF bsdf(ShadingFrame(record.surface().normal));
        bsdf.add(std::make_unique<LambertianBxDF>(Color(0.2f, 0.2f, 0.2f)));
        bsdf.add(std::make_unique<SpecularReflectionBxDF>(Color(0.3f, 0.3f, 0.3f)));
        return bsdf;
    }
};

/** Exposes a Volume Source without Pretending It Is Surface Radiance */
class TestEmissiveMedium final : public Material {
public:
    MaterialKind kind() const override { return MaterialKind::Medium; }
    bool isEmissive() const override { return true; }
    Color emission([[maybe_unused]] const HitRecord& record,
                   [[maybe_unused]] const Vec3f& outgoing) const override { return Color(1, 1, 1); }
    ScatteringResponse scattering(
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] TransportMode mode) const override {
        return PhaseResponse(Color(1, 1, 1), phase_);
    }
private:
    Isotropic phase_;
};

/** Deterministic Event Fixture; Isolates Emission Handling from Free-Flight Sampling */
class TestVolumeEvent final : public Intersectable {
public:
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             [[maybe_unused]] Sampler& sampler) const override {
        if (!ray_t.contains(0.5f)) return false;
        MediumRecord event;
        event.t = 0.5f;
        event.point = ray.at(event.t);
        event.outgoing = -unit_vector(ray.direction());
        event.material = &material_;
        record = event;
        return true;
    }
    const AABB& getBoundingBox() const override { return bounds_; }
private:
    TestEmissiveMedium material_;
    AABB bounds_{Point3f(-100, -100, -100), Point3f(100, 100, 100)};
};

void testUnsupportedVolumeEmission(nearlighter::test::Context& context) {
    Camera camera;
    camera.position = Point3f();
    camera.look_at = Point3f(0, 0, -1);
    RenderSettings settings;
    settings.image_width = settings.image_height = settings.samples_per_pixel = 1;
    settings.path.sampling = PathSamplingMode::ScatteringOnly;
    const Scene scene("volume source", camera, settings, Color(),
                      LinearAggregate(std::make_shared<TestVolumeEvent>()));
    bool rejected = false;
    try { (void)Renderer(settings).render(scene); }
    catch (const std::invalid_argument& error) {
        rejected = std::string(error.what()).find("Volume emission") != std::string::npos;
    }
    context.expectTrue(rejected, "volume source needs a distance estimator, not surface emission addition");
}

void testEstimatorMeans(nearlighter::test::Context& context) {
    Camera camera;
    camera.position = Point3f();
    camera.look_at = Point3f(0, 0, -1);
    camera.vertical_fov = 10;
    RenderSettings settings;
    settings.image_width = settings.image_height = 1;
    settings.samples_per_pixel = 32768;
    settings.path.max_depth = 4;
    settings.seed = 137;
    settings.aovs = FilmAOV::All;
    const auto target = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 100),
        std::make_shared<Lambertian>(Color(1, 1, 1)));

    for (const auto& material : std::vector<std::shared_ptr<const Material>>{
            std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f)),
            std::make_shared<MixedSurface>()}) {
        const auto plane = std::make_shared<Primitive>(
            std::make_shared<Quad>(Point3f(-100, -100, -1), Vec3f(200, 0, 0), Vec3f(0, 200, 0)), material);
        const Scene scene("analytic surface", camera, settings, Color(1, 1, 1),
                          LinearAggregate(plane), {target});
        for (auto mode : {PathSamplingMode::ScatteringOnly, PathSamplingMode::Mixture,
                          PathSamplingMode::LightOnly, PathSamplingMode::MIS}) {
            settings.path.sampling = mode;
            const auto result = Renderer(settings).render(scene);
            // Constant illumination integrates the diffuse and delta coefficients directly.
            context.expectVecNear(result.image().at(0, 0), Color(0.5f, 0.5f, 0.5f), 0.015f,
                                  "all estimators retain analytic surface energy including mixed delta PMFs");
            context.expectVecNear(result.film.direct().at(0, 0), result.image().at(0, 0), 1e-6f,
                                  "single surface scattering belongs to the direct AOV");
        }
    }

    for (float albedo : {0.0f, 0.6f, 1.0f}) {
        const auto medium = std::make_shared<Primitive>(
            std::make_shared<Sphere>(Point3f(), 1),
            std::make_shared<Homogeneous>(0.7f, Color(albedo, albedo, albedo)));
        const Scene scene("analytic medium", camera, settings, Color(1, 1, 1),
                          LinearAggregate(medium), {target});
        settings.path.max_depth = 32;
        Color scattering_mean;
        for (auto mode : {PathSamplingMode::ScatteringOnly, PathSamplingMode::Mixture}) {
            settings.path.sampling = mode;
            const auto result = Renderer(settings).render(scene);
            const Color beauty = result.image().at(0, 0);
            context.expectTrue(result.stats.counters.medium_interactions > 1000 && result.stats.counters.surface_interactions == 0,
                               "unified path actually queries volume events");
            context.expectTrue(result.stats.counters.shadow_rays == 0 && result.stats.counters.invalid_contributions == 0,
                               "volume continuation needs no unimplemented shadow estimator");
            context.expectVecNear(beauty, result.film.emission().at(0, 0) +
                result.film.direct().at(0, 0) + result.film.indirect().at(0, 0), 0.0002f,
                "volume paths preserve the AOV identity");
            if (albedo == 0) context.expectNear(beauty.x(), std::exp(-0.7f), 0.012f,
                                               "absorbing sphere matches analytic Beer transmittance");
            if (albedo == 1) context.expectNear(beauty.x(), 1, 0.002f,
                                               "conservative medium in uniform illumination conserves energy");
            if (mode == PathSamplingMode::ScatteringOnly) scattering_mean = beauty;
            else context.expectVecNear(beauty, scattering_mean, 0.015f,
                                       "volume mixture and phase-only estimates have matching means");
        }
        for (auto mode : {PathSamplingMode::LightOnly, PathSamplingMode::MIS}) {
            settings.path.sampling = mode;
            bool rejected = false;
            try { (void)Renderer(settings).render(scene); }
            catch (const std::invalid_argument&) { rejected = true; }
            context.expectTrue(rejected, "volume NEE fails before camera sampling");
        }
    }
}
}  // namespace

int main() {
    nearlighter::test::Context context;
    testRenderer(context);
    testPathIntegrator(context);
    testEstimatorMeans(context);
    testUnsupportedVolumeEmission(context);
    return context.finish("renderer smoke tests");
}
