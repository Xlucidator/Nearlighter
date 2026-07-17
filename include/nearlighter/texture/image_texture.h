#ifndef NEARLIGHTER_TEXTURE_IMAGE_TEXTURE_H
#define NEARLIGHTER_TEXTURE_IMAGE_TEXTURE_H

#include <nearlighter/base/image.h>
#include <nearlighter/texture/texture.h>

/** Samples an already-decoded linear RGB Image as a surface texture. */
class ImageTexture : public Texture {
public:
    explicit ImageTexture(Image image);

    Color value(float u, float v, const Point3f& p) const override;

private:
    Image image_;
};

#endif  // NEARLIGHTER_TEXTURE_IMAGE_TEXTURE_H
