#include <nearlighter/io/image_io.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
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

bool isLittleEndian() {
    const std::uint16_t value = 1;
    return *reinterpret_cast<const unsigned char*>(&value) == 1;
}

void writeLittleEndianFloat(std::ostream& output, float value) {
    static_assert(sizeof(float) == sizeof(std::uint32_t),
                  "PFM output requires 32-bit float storage");

    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(value));
    if (!isLittleEndian()) {
        bits = ((bits & 0x000000ffu) << 24u) |
               ((bits & 0x0000ff00u) << 8u) |
               ((bits & 0x00ff0000u) >> 8u) |
               ((bits & 0xff000000u) >> 24u);
    }
    output.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
}

}  // namespace

// ==================================================
// Image Loading
// ==================================================

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

// ==================================================
// PPM Output
// ==================================================

void PPMWriter::write(const Image& image) {
    if (finished_) {
        throw std::logic_error("Cannot write to a finished PPM output");
    }
    if (image.width() != width_ || image.height() != height_) {
        throw std::invalid_argument(
            "PPM source image dimensions do not match the output");
    }

    while (next_row_ < height_) {
        writeRow(image, next_row_);
    }
}

PPMWriter::PPMWriter(const std::filesystem::path& path, int width, int height,
                     const PPMWriteOptions& options)
    : path_(path), width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("PPM dimensions must be positive");
    }
    if (!std::isfinite(options.gamma) || options.gamma <= 0.0f) {
        throw std::invalid_argument("PPM gamma must be positive");
    }
    if (!std::isfinite(options.flush_interval_seconds) ||
        options.flush_interval_seconds <= 0.0) {
        throw std::invalid_argument("PPM flush interval must be positive");
    }
    inverse_gamma_ = 1.0f / options.gamma;
    flush_interval_ =
        std::chrono::duration<double>(options.flush_interval_seconds);

    output_.open(path_);
    if (!output_) {
        throw std::runtime_error("Failed to open PPM output '" +
                                 path_.string() + "'");
    }
    output_ << "P3\n" << width_ << ' ' << height_ << "\n255\n";
    if (!output_) {
        throw std::runtime_error("Failed while writing PPM header: " +
                                 path_.string());
    }
    last_flush_time_ = std::chrono::steady_clock::now();
}

void PPMWriter::writeRow(const Image& image, int row) {
    if (finished_) {
        throw std::logic_error("Cannot write to a finished PPM output");
    }
    if (image.width() != width_ || image.height() != height_) {
        throw std::invalid_argument(
            "PPM source image dimensions do not match the output");
    }
    if (row != next_row_) {
        throw std::invalid_argument("PPM rows must be written in order");
    }

    const std::size_t row_offset =
        static_cast<std::size_t>(row) * static_cast<std::size_t>(width_);
    for (int column = 0; column < width_; ++column) {
        const Color& color =
            image.pixels()[row_offset + static_cast<std::size_t>(column)];
        output_ << encodeChannel(color.x(), inverse_gamma_) << ' '
                << encodeChannel(color.y(), inverse_gamma_) << ' '
                << encodeChannel(color.z(), inverse_gamma_) << '\n';
    }

    if (!output_) {
        throw std::runtime_error("Failed while writing PPM output '" +
                                 path_.string() + "'");
    }
    ++next_row_;

    if (std::chrono::steady_clock::now() - last_flush_time_ >=
        flush_interval_) {
        flush();
    }
}

void PPMWriter::flush() {
    if (finished_) return;

    output_.flush();
    if (!output_) {
        throw std::runtime_error("Failed while flushing PPM output '" +
                                 path_.string() + "'");
    }
    last_flush_time_ = std::chrono::steady_clock::now();
}

void PPMWriter::finish() {
    if (finished_) return;
    if (next_row_ != height_) {
        throw std::logic_error("Cannot finish an incomplete PPM output");
    }

    output_.flush();
    output_.close();
    if (!output_) {
        throw std::runtime_error("Failed while closing PPM output '" +
                                 path_.string() + "'");
    }
    finished_ = true;
}

// ==================================================
// PFM Output
// ==================================================

PFMWriter::PFMWriter(const std::filesystem::path& path) : path_(path) {}

void PFMWriter::write(const Image& image) const {
    if (image.empty() || image.width() <= 0 || image.height() <= 0) {
        throw std::invalid_argument("Cannot write an empty PFM image");
    }

    std::ofstream output(path_, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Failed to open PFM output '" +
                                 path_.string() + "'");
    }

    /* A negative scale marks the payload as little-endian. */
    output << "PF\n" << image.width() << ' ' << image.height() << "\n-1.0\n";

    // PFM rows run from the bottom of the image to the top.
    for (int y = image.height() - 1; y >= 0; --y) {
        for (int x = 0; x < image.width(); ++x) {
            const Color& color = image.at(x, y);
            writeLittleEndianFloat(output, color.x());
            writeLittleEndianFloat(output, color.y());
            writeLittleEndianFloat(output, color.z());
        }
    }

    output.close();
    if (!output) {
        throw std::runtime_error("Failed while writing PFM output '" +
                                 path_.string() + "'");
    }
}
