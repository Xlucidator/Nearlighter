#ifndef NEARLIGHTER_IO_IMAGE_IO_H
#define NEARLIGHTER_IO_IMAGE_IO_H

#include <nearlighter/base/image.h>

#include <chrono>
#include <filesystem>
#include <fstream>

/** Describes how encoded source pixels must be converted to linear RGB. */
enum class SourceColorSpace {
    Srgb,
    Linear,
};

/** Controls decoding of an image file into core Image storage. */
struct ImageLoadOptions {
    SourceColorSpace source_color_space = SourceColorSpace::Srgb;
};

// ============================================================================
// PPM Output
//   PPMWriter supports complete-image and incremental row output
// ============================================================================

/** PPM encoding and streaming options */
struct PPMWriteOptions {
    float gamma = 2.2f;
    double flush_interval_seconds = 1.0;
};

/** Ordered ASCII PPM row writer */
class PPMWriter {
public:
    /**
     * Writes all remaining rows from a linear RGB Image.
     *
     * @throws std::logic_error after the output has been finished.
     * @throws std::invalid_argument for mismatched image dimensions.
     * @throws std::runtime_error when the output cannot be written.
     */
    void write(const Image& image);

    /** Opens an ordered row stream and writes its PPM header */
    PPMWriter(const std::filesystem::path& path, int width, int height,
              const PPMWriteOptions& options = {});

    PPMWriter(const PPMWriter&) = delete;
    PPMWriter& operator=(const PPMWriter&) = delete;

    /** Writes the next row and flushes when the configured interval expires */
    void writeRow(const Image& image, int row);

    /** Publishes buffered rows to the output file */
    void flush();

    /** Validates row completion and closes the output file */
    void finish();

private:
    std::filesystem::path path_;
    std::ofstream output_;
    int width_ = 0;
    int height_ = 0;
    int next_row_ = 0;
    float inverse_gamma_ = 1.0f;
    std::chrono::duration<double> flush_interval_{};
    std::chrono::steady_clock::time_point last_flush_time_{};
    bool finished_ = false;
};

/**
 * Loads an image file into linear floating-point RGB storage.
 *
 * @throws std::runtime_error when the file cannot be decoded.
 */
Image loadImage(const std::filesystem::path& path,
                const ImageLoadOptions& options = {});

#endif  // NEARLIGHTER_IO_IMAGE_IO_H
