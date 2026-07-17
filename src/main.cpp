#include "legacy_scenes.h"

#include <nearlighter/io/image_io.h>
#include <nearlighter/render/renderer.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int parseSceneSelection(int argc, char* argv[]) {
    if (argc == 1) return 6;
    if (argc == 3 &&
        (std::string(argv[1]) == "-s" ||
         std::string(argv[1]) == "--scene")) {
        return std::stoi(argv[2]);
    }

    throw std::invalid_argument(
        "Usage: Nearlighter [-s|--scene <number 0-9>]");
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const int selection = parseSceneSelection(argc, argv);
        Scene scene = makeLegacyScene(selection);
        Renderer renderer(scene.defaultRenderSettings());

        std::clog << "Rendering '" << scene.name() << "'...\n";
        RenderResult result = renderer.render(scene);
        writePPM("out.ppm", result.image);

        std::clog << "Render: " << result.stats.wall_time.count() << " s, "
                  << result.stats.samples_per_second << " samples/s\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
