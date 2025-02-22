#ifndef IMAGE_H
#define IMAGE_H

#include "texture.h"
#include <string>

class ImageTexture {
public:
    ImageTexture(const std::string& filename);
    Color value(float u, float v, const Point3f& p) const {

    }
};

#endif
