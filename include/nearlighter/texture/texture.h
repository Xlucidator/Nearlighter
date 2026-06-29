#ifndef TEXTURE_H
#define TEXTURE_H

#include <nearlighter/nearlighter.h>

class Texture {
public:
    virtual ~Texture() = default;
    virtual Color value(float u, float v, const Point3f& p) const = 0;
};

#endif // TEXTURE_H
