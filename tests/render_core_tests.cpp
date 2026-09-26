#include "test_support.h"

#include <nearlighter/base/interval.h>
#include <nearlighter/material/emissive.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/material/metal.h>
#include <nearlighter/render/film.h>
#include <nearlighter/light/area_light.h>
#include <nearlighter/light/environment_light.h>
#include <nearlighter/light/light_sampler.h>
#include <nearlighter/render/integrator.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/scene.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/shape/quad.h>
#include <nearlighter/shape/sphere.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kTolerance = 1e-5f;

void testFilmAccumulation(nearlighter::test::Context& context) {
    Film film(2, 1);
    film.addSample(
        0, 0,
        FilmSample::fromComponents(Color(1.0f, 2.0f, 3.0f),
                                   Color(2.0f, 0.0f, 0.0f),
                                   Color(0.0f, 1.0f, 0.0f)));
    film.addSample(
        0, 0,
        FilmSample::fromComponents(Color(3.0f, 2.0f, 1.0f),
                                   Color(0.0f, 2.0f, 0.0f),
                                   Color(0.0f, 0.0f, 2.0f)));

    context.expectVecNear(film.beauty().at(0, 0),
                          Color(3.0f, 3.5f, 3.0f), kTolerance,
                          "Film beauty should be the primary-sample mean");
    context.expectVecNear(film.emission().at(0, 0), Color(2.0f, 2.0f, 2.0f),
                          kTolerance,
                          "Film should retain mean emission separately");
    context.expectVecNear(film.direct().at(0, 0), Color(1.0f, 1.0f, 0.0f),
                          kTolerance,
                          "Film should retain mean direct lighting separately");
    context.expectVecNear(film.indirect().at(0, 0),
                          Color(0.0f, 0.5f, 1.0f), kTolerance,
                          "Film should retain mean indirect lighting separately");
    context.expectVecNear(film.variance().at(0, 0), Color(0.0f, 0.5f, 0.0f),
                          kTolerance,
                          "Film variance should use the unbiased sample estimate");
    context.expectTrue(film.sampleCountAt(0, 0) == 2,
                       "Film should retain an exact per-pixel sample count");

    film.addBeautySample(1, 0, Color(0.25f, 0.5f, 0.75f));
    context.expectVecNear(film.beauty().at(1, 0),
                          Color(0.25f, 0.5f, 0.75f), kTolerance,
                          "beauty-only accumulation should support legacy output");

    Film beauty_only(1, 1, FilmAOV::Beauty);
    bool disabled_layer_rejected = false;
    try {
        (void)beauty_only.direct();
    } catch (const std::logic_error&) {
        disabled_layer_rejected = true;
    }
    context.expectTrue(
        disabled_layer_rejected,
        "Film should reject access to an unallocated optional AOV");
}

SurfaceInteraction makeReferenceInteraction(
    const Primitive& primitive, Sampler& sampler) {
    const Ray ray(Point3f(0.0f, 0.0f, 3.0f), Vec3f(0.0f, 0.0f, -1.0f));
    HitRecord record;
    if (!primitive.hit(ray, Interval(0.0f, infinity), record, sampler)) {
        throw std::runtime_error("failed to create light-test interaction");
    }
    return SurfaceInteraction(ray, record);
}

void testLightSampling(nearlighter::test::Context& context) {
    const auto diffuse =
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f));
    const auto reference = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, 0.0f), 1.0f),
        diffuse);
    const Color radiance(4.0f, 5.0f, 6.0f);
    const auto emitter = std::make_shared<Primitive>(
        std::make_shared<Quad>(Point3f(-1.0f, 1.0f, 3.0f),
                               Vec3f(2.0f, 0.0f, 0.0f),
                               Vec3f(0.0f, -2.0f, 0.0f)),
        std::make_shared<Emissive>(radiance));

    Sampler sampler(27);
    const SurfaceInteraction interaction =
        makeReferenceInteraction(*reference, sampler);
    const Scene scene("area", Camera(), RenderSettings(), Color(), LinearAggregate(emitter));
    const LightSampler lights(scene.lights());
    const auto selected = lights.select(sampler.next1D());
    const auto sample = selected->light->sampleLi(interaction, 0.0f, sampler);
    context.expectTrue(sample.has_value(),
                       "area light should produce a direction sample");
    if (sample) {
        context.expectTrue(selected->light == scene.findAreaLights(*emitter),
                           "light sample should identify its emitter");
        context.expectTrue(sample->incoming.z() > 0.0f,
                           "sampled area light should lie above the reference");
        context.expectVecNear(sample->radiance, radiance, kTolerance,
                              "front-facing area light should emit its radiance");
        context.expectTrue(sample->pdf > 0.0f && sample->distance > 0.0f,
                           "finite light sample should provide density and distance");
        context.expectNear(
            lights.PMF(*selected->light) * selected->light->PDFLi(interaction, sample->incoming),
            sample->pdf, kTolerance,
            "area-light PDF query should match its one-light sampling strategy");
    }

    const Scene sky("sky", Camera(), RenderSettings(), Color(0.1f, 0.2f, 0.3f), {});
    const LightSampler environment(sky.lights());
    context.expectTrue(environment.select(0.5f)->light == sky.environment(),
                       "escape evaluation and light selection must share one environment");
    const auto environment_sample = sky.environment()->sampleLi(interaction, 0.0f, sampler);
    context.expectTrue(environment_sample.has_value() &&
                           std::isinf(environment_sample->distance),
                       "non-black environment should become an infinite light");
    if (environment_sample) {
        context.expectVecNear(environment_sample->radiance,
                              sky.environment()->evaluateLi(environment_sample->incoming),
                              kTolerance, "sampled and escaped environment radiance must agree");
        context.expectNear(environment_sample->pdf, 1.0f / (4.0f * pi),
                           kTolerance,
                           "constant environment should use a uniform-sphere PDF");
    }

    const Scene both("both", Camera(), RenderSettings(), Color(0.1f, 0.2f, 0.3f),
                     LinearAggregate(emitter));
    const LightSampler combined(both.lights());
    context.expectTrue(combined.select(0.0f)->light == both.findAreaLights(*emitter) &&
                           combined.select(0.75f)->light == both.environment(),
                       "uniform selection should partition the unit interval");
    context.expectNear(combined.PMF(*both.findAreaLights(*emitter)) +
                           combined.PMF(*both.environment()), 1.0f, kTolerance,
                       "selection probabilities should sum to one");
    context.expectNear(combined.PMF(*scene.findAreaLights(*emitter)), 0.0f, kTolerance,
                       "a light outside the collection has zero selection probability");
    const Scene empty("empty", Camera(), RenderSettings(), Color(), {});
    context.expectFalse(LightSampler(empty.lights()).select(0.0f).has_value(),
                        "empty scenes should have no light selection");
    if (sample) {
        context.expectNear(
            combined.PMF(*both.findAreaLights(*emitter)) * both.findAreaLights(*emitter)->PDFLi(interaction, sample->incoming),
            0.5f * sample->pdf, kTolerance,
            "two-light strategy should include a one-half selection PMF");
    }
    context.expectNear(
        combined.PMF(*both.environment()) * both.environment()->PDFLi(interaction, Vec3f(0.0f, 0.0f, 1.0f)),
        1.0f / (8.0f * pi), kTolerance,
        "environment PDF should include the same uniform selection PMF");
}

Camera makeCamera() {
    Camera camera;
    camera.position = Point3f(0.0f, 0.0f, 4.0f);
    camera.look_at = Point3f(0.0f, 0.0f, 0.0f);
    return camera;
}

void testPathSceneCompatibility(nearlighter::test::Context& context) {
    RenderSettings settings;
    settings.image_width = 4;
    settings.image_height = 4;

    LinearAggregate supported_world;
    supported_world.add(std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 1.0f),
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f))));
    supported_world.add(std::make_shared<Primitive>(
        std::make_shared<Quad>(Point3f(-1.0f, 1.0f, 2.0f),
                               Vec3f(2.0f, 0.0f, 0.0f),
                               Vec3f(0.0f, -2.0f, 0.0f)),
        std::make_shared<Emissive>(Color(2.0f, 2.0f, 2.0f))));
    const Scene supported("supported", makeCamera(), settings, Color(),
                          std::move(supported_world));
    PathIntegrator::validateScene(supported);
    context.expectTrue(supported.lights().size() == 1,
                       "Scene should already own its discovered light");

    LinearAggregate unsupported_world;
    unsupported_world.add(std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 1.0f),
        std::make_shared<Metal>(Color(0.8f, 0.8f, 0.8f), 0.2f)));
    const Scene unsupported("unsupported", makeCamera(), settings, Color(),
                            std::move(unsupported_world));
    bool rejected = false;
    try { PathIntegrator::validateScene(unsupported); }
    catch (const std::invalid_argument&) { rejected = true; }
    context.expectTrue(
        rejected,
        "fuzzy metal should be rejected before new path integration");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testFilmAccumulation(context);
    testLightSampling(context);
    testPathSceneCompatibility(context);
    return context.finish("render core tests");
}
