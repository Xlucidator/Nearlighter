#ifndef IMAGE_H
#define IMAGE_H

#include "texture.h"
#include "utils/imageloader.h"
#include "utils/math.h"

#include <string>

class ImageTexture : public Texture {
public:
    ImageTexture(const std::string& filename) 
         : _image("assets/textures/" + filename) { image_name = filename; }
        // string& can not receive a temporary string literal, but const string& can

    Color value(float u, float v, const Point3f& p) const override {
        if (!_image.hasData()) return Color(0, 1, 1); // cyan for debug

        // get legal u, v, clamp & map to [0, 1]x[1, 0]
        u = clamp_num<float>(u, 0.0, 1.0);
        v = 1.0 - clamp_num<float>(v, 0.0, 1.0); // TODO: whether use stbi_image_flip_vertically?

        // get color value
        int i = static_cast<int>(u * _image.width());
        int j = static_cast<int>(v * _image.height());
        const unsigned char* pixel = _image.getPixelData(i, j);

        float color_scale = 1.0 / 255.0;
        return Color(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
    }

private:
    ImageLoader _image;  // TODO: should it better to be a pointer?
    std::string image_name;
};

#endif
