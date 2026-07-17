#ifndef NEARLIGHTER_IO_IMAGE_IO_H
#define NEARLIGHTER_IO_IMAGE_IO_H

#include <nearlighter/base/image.h>

#include <filesystem>

/** Describes how encoded source pixels must be converted to linear RGB. */
enum class SourceColorSpace {
    Srgb,
    Linear,
};

/** Controls decoding of an image file into core Image storage. */
struct ImageLoadOptions {
    SourceColorSpace source_color_space = SourceColorSpace::Srgb;
};

/** Controls conversion of a linear Image to an 8-bit PPM preview. */
struct PpmWriteOptions {
    float gamma = 2.2f;
};

/**
 * Loads an image file into linear floating-point RGB storage.
 *
 * @throws std::runtime_error when the file cannot be decoded.
 */
Image loadImage(const std::filesystem::path& path,
                const ImageLoadOptions& options = {});

/**
 * Writes a linear floating-point image as an ASCII PPM file.
 *
 * @throws std::invalid_argument when gamma is not positive.
 * @throws std::runtime_error when the output file cannot be written.
 */
void writePPM(const std::filesystem::path& path, const Image& image,
              const PpmWriteOptions& options = {});

#endif  // NEARLIGHTER_IO_IMAGE_IO_H
