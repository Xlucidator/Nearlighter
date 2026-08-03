#ifndef CHECKER_H
#define CHECKER_H

#include <nearlighter/texture/texture.h>

#include <memory>

/* Spatial Checker Texture */
class CheckerTexture : public Texture {
public:
    CheckerTexture(float scale, std::shared_ptr<Texture> even, std::shared_ptr<Texture> odd);
    CheckerTexture(float scale, const Color& even, const Color& odd);

    Color value(float u, float v, const Point3f& p) const override;
    
private:
    float inv_scale;
    std::shared_ptr<Texture> even;
    std::shared_ptr<Texture> odd;
};

#endif // CHECKER_H
