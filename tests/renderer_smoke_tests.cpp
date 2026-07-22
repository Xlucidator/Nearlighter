#include "test_support.h"

#include <nearlighter/geometry/sphere.h>
#include <nearlighter/material/diffuse_light.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/scene.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>

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

    ShapeList world;
    world.add(std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -1.0f), 0.5f,
        std::make_shared<DiffuseLight>(Color(2.0f, 1.0f, 0.5f))));
    return Scene("Renderer smoke test", camera, settings,
                 Color(0.05f, 0.1f, 0.2f), std::move(world));
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
        [&](const RenderProgress& progress, const Image& partial_image) {
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

    context.expectTrue(first.image.width() == settings.image_width,
                       "rendered image width should match RenderSettings");
    context.expectTrue(first.image.height() == settings.image_height,
                       "rendered image height should match RenderSettings");
    context.expectTrue(imagesEqual(first.image, second.image),
                       "equal scene settings and seed should reproduce every pixel");
    context.expectTrue(progress_updates == settings.image_height,
                       "Renderer should report each completed image row");
    context.expectTrue(progress_contract_valid,
                       "render progress should be ordered and match the output image");

    bool non_black = false;
    bool all_finite = true;
    for (const Color& pixel : first.image.pixels()) {
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
    context.expectFalse(imagesEqual(first.image, changed.image),
                        "changing the render seed should change sampled pixels");
}

void testInvalidConfiguration(nearlighter::test::Context& context) {
    RenderSettings invalid_settings;
    invalid_settings.image_width = 0;
    bool settings_rejected = false;
    try {
        Renderer renderer(invalid_settings);
    } catch (const std::invalid_argument&) {
        settings_rejected = true;
    }
    context.expectTrue(settings_rejected,
                       "Renderer should reject non-positive dimensions");

    Camera invalid_camera;
    invalid_camera.look_at = invalid_camera.position;
    bool camera_rejected = false;
    try {
        invalid_camera.prepare(8, 8);
    } catch (const std::invalid_argument&) {
        camera_rejected = true;
    }
    context.expectTrue(camera_rejected,
                       "Camera should reject a degenerate view direction");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testRenderer(context);
    testInvalidConfiguration(context);
    return context.finish("renderer smoke tests");
}
