#include "test_support.h"

#include <nearlighter/material/emissive.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/primitive.h>
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
    settings.max_depth = 4;
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
    settings.max_depth = 4;
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
    const RenderSettings settings = scene.defaultRenderSettings();
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
    context.expectTrue(first.stats.sample_count == expected_samples,
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
    const RenderSettings settings = scene.defaultRenderSettings();
    RenderOptions options;
    options.integrator = IntegratorKind::Path;
    options.path.direct_lighting = DirectLightingMode::MIS;
    options.aovs = FilmAOV::All;

    const RenderResult first = Renderer(settings, options).render(scene);
    const RenderResult second = Renderer(settings, options).render(scene);
    context.expectTrue(imagesEqual(first.image(), second.image()),
                       "path integrator should preserve deterministic sampling");
    context.expectTrue(first.stats.shadow_rays > 0,
                       "MIS path render should cast explicit shadow rays");
    context.expectTrue(first.stats.continuation_rays > 0,
                       "path render should sample BSDF continuation rays");
    context.expectTrue(first.stats.russian_roulette_terminations == 0,
                       "Russian roulette should be disabled by default");
    context.expectTrue(first.stats.invalid_pdf_terminations == 0 &&
                           first.stats.invalid_contributions == 0,
                       "supported diffuse path scene should avoid invalid events");

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

    RenderOptions bsdf_only_options = options;
    bsdf_only_options.path.direct_lighting = DirectLightingMode::BSDFOnly;
    const RenderResult bsdf_only =
        Renderer(settings, bsdf_only_options).render(scene);
    context.expectTrue(bsdf_only.stats.shadow_rays == 0,
                       "BSDF-only mode should disable next-event estimation");

    RenderOptions light_only_options = options;
    light_only_options.path.direct_lighting = DirectLightingMode::LightOnly;
    const RenderResult light_only =
        Renderer(settings, light_only_options).render(scene);
    context.expectTrue(light_only.stats.shadow_rays > 0,
                       "light-only mode should retain next-event estimation");

    RenderOptions roulette_options = options;
    roulette_options.path.russian_roulette_start_depth = 1;
    const RenderResult roulette =
        Renderer(settings, roulette_options).render(scene);
    context.expectTrue(roulette.stats.russian_roulette_terminations > 0,
                       "enabled Russian roulette should terminate some paths");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testRenderer(context);
    testPathIntegrator(context);
    return context.finish("renderer smoke tests");
}
