#include <nearlighter/render/renderer.h>

#include <nearlighter/accel/bvh_node.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/material/material.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/scene.h>

#include <memory>
#include <stdexcept>
#include <utility>


Renderer::Renderer(RenderSettings settings) : settings_(settings) {
    if (settings.image_width <= 0 || settings.image_height <= 0) {
        throw std::invalid_argument("Render image dimensions must be positive");
    }
    if (settings.samples_per_pixel <= 0) {
        throw std::invalid_argument("Render samples per pixel must be positive");
    }
    if (settings.max_depth <= 0) {
        throw std::invalid_argument("Render maximum path depth must be positive");
    }
}

RenderResult Renderer::render(const Scene& scene) const {
    const Camera::Prepared camera =
        scene.camera().prepare(settings_.image_width, settings_.image_height);
    Image image(settings_.image_width, settings_.image_height);

    /* Build an acceleration root without mutating the Scene's object order. */
    ShapeList world_copy;
    std::unique_ptr<BVHNode> bvh;
    const Shape* world = &scene.world();
    if (scene.world().size() > 1) {
        world_copy = scene.world();
        bvh = std::make_unique<BVHNode>(world_copy);
        world = bvh.get();
    }

    const auto start_time = std::chrono::steady_clock::now();
    for (int y = 0; y < settings_.image_height; ++y) {
        for (int x = 0; x < settings_.image_width; ++x) {
            Color pixel(0.0f, 0.0f, 0.0f);

            // Each primary sample owns an independent deterministic sequence.
            for (int sample_index = 0;
                 sample_index < settings_.samples_per_pixel; ++sample_index) {
                Sampler sampler(derivePathSeed(
                    settings_.seed, static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    static_cast<std::uint32_t>(sample_index)));
                const Ray ray = camera.generateRay(x, y, sampler);
                pixel += trace(ray, settings_.max_depth, *world,
                               scene.samplingTargets(), scene.background(),
                               sampler);
            }

            image.at(x, y) =
                pixel / static_cast<float>(settings_.samples_per_pixel);
        }
    }
    const auto end_time = std::chrono::steady_clock::now();

    RenderStats stats;
    stats.wall_time = end_time - start_time;
    stats.sample_count =
        static_cast<std::uint64_t>(settings_.image_width) *
        static_cast<std::uint64_t>(settings_.image_height) *
        static_cast<std::uint64_t>(settings_.samples_per_pixel);
    if (stats.wall_time.count() > 0.0) {
        stats.samples_per_second =
            static_cast<double>(stats.sample_count) / stats.wall_time.count();
    }

    return RenderResult{std::move(image), stats};
}

Color Renderer::trace(const Ray& ray, int depth, const Shape& world,
                      const Shape& sampling_targets,
                      const Color& background, Sampler& sampler) const {
    if (depth <= 0) return Color(0.0f, 0.0f, 0.0f);

    HitRecord record;
    if (!world.hit(ray, Interval(0.001f, infinity), record, sampler)) {
        return background;
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
        return emitted + scatter_record.attenuation *
                             trace(scatter_record.skip_ray, depth - 1, world,
                                   sampling_targets, background, sampler);
    }
    if (!scatter_record.pdf) return emitted;

    /* Mix explicit target sampling with the material distribution when present. */
    shared_ptr<PDF> sample_pdf = scatter_record.pdf;
    if (sampling_targets.hasPDF()) {
        auto target_pdf = make_shared<ShapePDF>(sampling_targets, record.point);
        sample_pdf = make_shared<MixturePDF>(target_pdf, scatter_record.pdf);
    }

    const Ray scattered(record.point, sample_pdf->generate(sampler), ray.time());
    const float pdf_value = sample_pdf->value(scattered.direction());
    if (pdf_value <= 0.0f) return emitted;

    const float scattering_pdf =
        record.material->getScatterPDF(ray, record, scattered);
    const Color incoming = trace(scattered, depth - 1, world,
                                 sampling_targets, background, sampler);
    return emitted + scatter_record.attenuation * scattering_pdf * incoming /
                         pdf_value;
}
