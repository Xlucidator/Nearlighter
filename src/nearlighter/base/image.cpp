#include <nearlighter/base/image.h>

#include <stdexcept>

Image::Image(int width, int height, const Color& initial_value)
    : width_(width), height_(height) {
    if (width < 0 || height < 0) {
        throw std::invalid_argument("Image dimensions must not be negative");
    }

    pixels_.assign(static_cast<std::size_t>(width) *
                       static_cast<std::size_t>(height),
                   initial_value);
}

Color& Image::at(int x, int y) {
    return pixels_[index(x, y)];
}

const Color& Image::at(int x, int y) const {
    return pixels_[index(x, y)];
}

std::size_t Image::index(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        throw std::out_of_range("Image pixel coordinates are outside the image");
    }

    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}
