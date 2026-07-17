#ifndef NEARLIGHTER_BASE_IMAGE_H
#define NEARLIGHTER_BASE_IMAGE_H

#include <nearlighter/base/color.h>

#include <cstddef>
#include <vector>

/**
 * Stores a two-dimensional linear RGB image in row-major order.
 *
 * Pixels are ordered from left to right and from top to bottom. Color values
 * are not clamped or gamma encoded; file-format conversion belongs to the I/O
 * layer.
 */
class Image {
public:
    Image() = default;

    /** Constructs an image and initializes every pixel to the supplied value. */
    Image(int width, int height, const Color& initial_value = Color());

    /** Returns the image width in pixels. */
    int width() const { return width_; }

    /** Returns the image height in pixels. */
    int height() const { return height_; }

    /** Returns whether the image contains no pixels. */
    bool empty() const { return pixels_.empty(); }

    /** Returns the pixel at `(x, y)` and checks that the coordinates are valid. */
    Color& at(int x, int y);

    /** Returns the pixel at `(x, y)` and checks that the coordinates are valid. */
    const Color& at(int x, int y) const;

    /** Returns the contiguous row-major pixel storage. */
    const std::vector<Color>& pixels() const { return pixels_; }

private:
    std::size_t index(int x, int y) const;

    int width_ = 0;
    int height_ = 0;
    std::vector<Color> pixels_;
};

#endif  // NEARLIGHTER_BASE_IMAGE_H
