#include <nearlighter/texture/image_texture.h>

#include <nearlighter/math/math.h>

ImageTexture::ImageTexture(const std::string& filename)
    : _image("assets/textures/" + filename), image_name(filename) {}

Color ImageTexture::value(float u, float v, const Point3f&) const {
    if (!_image.hasData()) return Color(0, 1, 1);

    // Clamp texture coordinates and map v from image coordinates to texture coordinates.
    u = clamp_num<float>(u, 0.0f, 1.0f);
    v = 1.0f - clamp_num<float>(v, 0.0f, 1.0f);

    int i = static_cast<int>(u * _image.width());
    int j = static_cast<int>(v * _image.height());
    const unsigned char* pixel = _image.getPixelData(i, j);

    float color_scale = 1.0f / 255.0f;
    return Color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
}
