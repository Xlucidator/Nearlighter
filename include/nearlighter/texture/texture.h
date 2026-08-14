#ifndef TEXTURE_H
#define TEXTURE_H

#include <nearlighter/base/color.h>

/** Spatial or parameterized color source evaluated by Materials. */
class Texture {
public:
    virtual ~Texture() = default;

    /**
     * Evaluates the texture at one surface location.
     *
     * @param u Horizontal texture coordinate; interpretation is
     * texture-specific.
     * @param v Vertical texture coordinate; interpretation is
     * texture-specific.
     * @param p Surface point in the coordinate system expected by the texture.
     * @return Linear RGB texture value.
     */
    virtual Color value(float u, float v, const Point3f& p) const = 0;
};

#endif // TEXTURE_H
