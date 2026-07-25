#include "legacy_scenes.h"

#include <nearlighter/io/console_io.h>
#include <nearlighter/io/image_io.h>
#include <nearlighter/io/scene_loader.h>
#include <nearlighter/render/renderer.h>

#include <argparse/argparse.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

// ==================================================
// Command-line Data
// ==================================================

/** CLI-only application options */
struct CliOptions {
    std::filesystem::path scene_path = "cornell_box_rtow.json";
    std::filesystem::path output_path = "out.ppm";
    std::optional<std::filesystem::path> linear_output_path;
    std::optional<int> legacy_scene;
    std::optional<int> image_width;
    std::optional<int> image_height;
    std::optional<int> samples_per_pixel;
    std::optional<int> max_depth;
    std::optional<std::uint64_t> seed;
    bool show_progress = true;
    double flush_interval_seconds = 1.0;
};

// ==================================================
// Command-line Processing
// ==================================================

/**
 * Resolves bundled scene filenames without changing explicit paths.
 *
 * Bare filename: executable/assets/scenes/<filename>.
 * Absolute or directory-qualified path: unchanged.
 */
std::filesystem::path resolveScenePath(
    const std::filesystem::path& scene_path,
    const std::filesystem::path& executable_path) {
    if (scene_path.is_absolute() || scene_path.has_parent_path()) {
        return scene_path;
    }

    const std::filesystem::path executable_directory =
        std::filesystem::absolute(executable_path).parent_path();
    return executable_directory / "assets" / "scenes" / scene_path;
}

/** Parses command-line options and prints library-generated help on failure */
CliOptions parseCommandLine(int argc, char* argv[]) {
    argparse::ArgumentParser program("Nearlighter", "0.1.0");
    program.add_description("CPU path tracer");
    program.add_argument("-s", "--scene")
        .help("load a bundled scene filename or an explicit JSON path")
        .default_value(std::string("cornell_box_rtow.json"));
    program.add_argument("--legacy-scene")
        .help("select a temporary legacy C++ scene from 0 to 9")
        .scan<'i', int>()
        .choices(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    program.add_argument("-o", "--output")
        .help("write the display-ready PPM image to this path")
        .default_value(std::string("out.ppm"));
    program.add_argument("--linear-output")
        .help("also write the linear RGB image as PFM");
    program.add_argument("--width")
        .help("override the scene image width")
        .scan<'i', int>();
    program.add_argument("--height")
        .help("override the scene image height")
        .scan<'i', int>();
    program.add_argument("--spp")
        .help("override samples per pixel")
        .scan<'i', int>();
    program.add_argument("--max-depth")
        .help("override the maximum path depth")
        .scan<'i', int>();
    program.add_argument("--seed")
        .help("override the deterministic render seed")
        .scan<'u', unsigned long long>();
    program.add_argument("--no-progress")
        .help("disable terminal render progress")
        .flag();
    program.add_argument("--flush-interval")
        .help("set output flush interval in seconds")
        .default_value(1.0)
        .scan<'g', double>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& error) {
        throw std::invalid_argument(
            std::string(error.what()) + "\n\n" + program.help().str());
    }

    if (program.is_used("--scene") &&
        program.is_used("--legacy-scene")) {
        throw std::invalid_argument(
            "--scene and --legacy-scene cannot be used together\n\n" +
            program.help().str());
    }

    std::optional<int> legacy_scene;
    if (program.is_used("--legacy-scene")) {
        legacy_scene = program.get<int>("--legacy-scene");
    }

    CliOptions options;
    options.scene_path = program.get<std::string>("--scene");
    options.output_path = program.get<std::string>("--output");
    options.legacy_scene = legacy_scene;
    options.show_progress = !program.get<bool>("--no-progress");
    options.flush_interval_seconds =
        program.get<double>("--flush-interval");

    if (program.is_used("--linear-output")) {
        options.linear_output_path =
            program.get<std::string>("--linear-output");
    }
    if (program.is_used("--width")) {
        options.image_width = program.get<int>("--width");
    }
    if (program.is_used("--height")) {
        options.image_height = program.get<int>("--height");
    }
    if (program.is_used("--spp")) {
        options.samples_per_pixel = program.get<int>("--spp");
    }
    if (program.is_used("--max-depth")) {
        options.max_depth = program.get<int>("--max-depth");
    }
    if (program.is_used("--seed")) {
        options.seed = static_cast<std::uint64_t>(
            program.get<unsigned long long>("--seed"));
    }

    return options;
}

/** Applies only command-line values explicitly supplied by the caller. */
void applyRenderOverrides(RenderSettings& settings,
                          const CliOptions& options) {
    if (options.image_width) settings.image_width = *options.image_width;
    if (options.image_height) settings.image_height = *options.image_height;
    if (options.samples_per_pixel) {
        settings.samples_per_pixel = *options.samples_per_pixel;
    }
    if (options.max_depth) settings.max_depth = *options.max_depth;
    if (options.seed) settings.seed = *options.seed;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const CliOptions options = parseCommandLine(argc, argv);
        const std::filesystem::path scene_path =
            resolveScenePath(options.scene_path, argv[0]);
        Scene scene = options.legacy_scene
                          ? makeLegacyScene(*options.legacy_scene)
                          : SceneLoader().load(scene_path);
        RenderSettings render_settings = scene.defaultRenderSettings();
        applyRenderOverrides(render_settings, options);
        Renderer renderer(render_settings);

        const RenderSettings& settings = renderer.settings();
        PPMWriteOptions output_options;
        output_options.flush_interval_seconds = options.flush_interval_seconds;
        PPMWriter image_output(options.output_path, settings.image_width,
                               settings.image_height, output_options);
        ConsoleOutput console_output(std::clog, options.show_progress);
        console_output.beginRender(scene.name());

        RenderResult result = renderer.render(
            scene,
            [&](const RenderProgress& progress, const Image& image) {
                image_output.writeRow(image, progress.completed_rows - 1);
                console_output.updateRender(progress);
            });

        image_output.write(result.image);
        image_output.finish();
        if (options.linear_output_path) {
            PFMWriter(*options.linear_output_path).write(result.image);
        }
        console_output.finishRender(result.stats);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
