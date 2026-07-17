#include <nearlighter/io/image_io.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

constexpr int kRgbChannels = 3;

float srgbToLinear(float value) {
    if (value <= 0.04045f) return value / 12.92f;
    return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

int encodeChannel(float linear_value, float inverse_gamma) {
    if (!std::isfinite(linear_value)) linear_value = 0.0f;
    const float clamped = std::clamp(linear_value, 0.0f, 1.0f);
    const float encoded = std::pow(clamped, inverse_gamma);
    return static_cast<int>(std::clamp(encoded, 0.0f, 0.999f) * 256.0f);
}

}  // namespace

Image loadImage(const std::filesystem::path& path,
                const ImageLoadOptions& options) {
    int width = 0;
    int height = 0;
    int source_channels = 0;
    unsigned char* decoded = stbi_load(path.string().c_str(), &width, &height,
                                       &source_channels, kRgbChannels);
    if (!decoded) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load image '" + path.string() + "': " +
            (reason ? reason : "unknown stb error"));
    }

    const std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
        decoded, &stbi_image_free);
    Image image(width, height);
    constexpr float kByteScale = 1.0f / 255.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t offset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                 static_cast<std::size_t>(x)) *
                kRgbChannels;
            Color color(kByteScale * data.get()[offset],
                        kByteScale * data.get()[offset + 1],
                        kByteScale * data.get()[offset + 2]);
            if (options.source_color_space == SourceColorSpace::Srgb) {
                color = Color(srgbToLinear(color.x()), srgbToLinear(color.y()),
                              srgbToLinear(color.z()));
            }
            image.at(x, y) = color;
        }
    }

    return image;
}

void writePPM(const std::filesystem::path& path, const Image& image,
              const PpmWriteOptions& options) {
    if (options.gamma <= 0.0f) {
        throw std::invalid_argument("PPM gamma must be positive");
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Failed to open PPM output '" +
                                 path.string() + "'");
    }

    output << "P3\n" << image.width() << ' ' << image.height() << "\n255\n";
    const float inverse_gamma = 1.0f / options.gamma;
    for (const Color& color : image.pixels()) {
        output << encodeChannel(color.x(), inverse_gamma) << ' '
               << encodeChannel(color.y(), inverse_gamma) << ' '
               << encodeChannel(color.z(), inverse_gamma) << '\n';
    }

    if (!output) {
        throw std::runtime_error("Failed while writing PPM output '" +
                                 path.string() + "'");
    }
}
