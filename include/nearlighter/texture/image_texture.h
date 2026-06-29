#ifndef IMAGE_H
#define IMAGE_H

#include <nearlighter/texture/texture.h>
#include <nearlighter/io/image_loader.h>

#include <string>

class ImageTexture : public Texture {
public:
    ImageTexture(const std::string& filename);

    Color value(float u, float v, const Point3f& p) const override;

private:
    ImageLoader _image;  // TODO: should it better to be a pointer?
    std::string image_name;
};

#endif
