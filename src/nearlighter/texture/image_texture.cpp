#include <nearlighter/texture/image_texture.h>

#include <algorithm>
#include <utility>

ImageTexture::ImageTexture(Image image) : image_(std::move(image)) {}

Color ImageTexture::value(float u, float v, const Point3f&) const {
    if (image_.empty()) return Color(0.0f, 1.0f, 1.0f);

    // Texture v points upward while image rows are stored from top to bottom.
    u = std::clamp(u, 0.0f, 1.0f);
    v = 1.0f - std::clamp(v, 0.0f, 1.0f);
    const int x = std::min(static_cast<int>(u * image_.width()),
                           image_.width() - 1);
    const int y = std::min(static_cast<int>(v * image_.height()),
                           image_.height() - 1);
    return image_.at(x, y);
}
