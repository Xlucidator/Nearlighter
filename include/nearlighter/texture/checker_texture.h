#ifndef CHECKER_H
#define CHECKER_H

#include <nearlighter/texture/texture.h>

/* Spatial Checker Texture */
class CheckerTexture : public Texture {
public:
    CheckerTexture(float scale, shared_ptr<Texture> even, shared_ptr<Texture> odd);
    CheckerTexture(float scale, const Color& even, const Color& odd);

    Color value(float u, float v, const Point3f& p) const override;
    
private:
    float inv_scale;
    shared_ptr<Texture> even;
    shared_ptr<Texture> odd;
};

#endif // CHECKER_H
