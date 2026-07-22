#include "legacy_scenes.h"

#include <nearlighter/io/console_io.h>
#include <nearlighter/io/image_io.h>
#include <nearlighter/io/scene_loader.h>
#include <nearlighter/render/renderer.h>

#include <argparse/argparse.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

/** CLI-only application options */
struct CliOptions {
    std::filesystem::path scene_path =
        "assets/scenes/cornell_box_rtow.json";
    std::optional<int> legacy_scene;
    bool show_progress = true;
    double flush_interval_seconds = 1.0;
};

/** Parses command-line options and prints library-generated help on failure */
CliOptions parseCommandLine(int argc, char* argv[]) {
    argparse::ArgumentParser program("Nearlighter", "0.1.0");
    program.add_description("CPU path tracer");
    program.add_argument("-s", "--scene")
        .help("load a JSON scene file")
        .default_value(std::string("assets/scenes/cornell_box_rtow.json"));
    program.add_argument("--legacy-scene")
        .help("select a temporary legacy C++ scene from 0 to 9")
        .scan<'i', int>()
        .choices(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
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

    return CliOptions{
        program.get<std::string>("--scene"),
        legacy_scene,
        !program.get<bool>("--no-progress"),
        program.get<double>("--flush-interval"),
    };
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const CliOptions options = parseCommandLine(argc, argv);
        Scene scene = options.legacy_scene
                          ? makeLegacyScene(*options.legacy_scene)
                          : SceneLoader().load(options.scene_path);
        Renderer renderer(scene.defaultRenderSettings());

        const RenderSettings& settings = renderer.settings();
        PPMWriteOptions output_options;
        output_options.flush_interval_seconds = options.flush_interval_seconds;
        PPMWriter image_output("out.ppm", settings.image_width,
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
        console_output.finishRender(result.stats);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
