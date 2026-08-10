#ifndef PDF_H
#define PDF_H 

#include <nearlighter/base/onb.h>
#include <nearlighter/geometry/shape.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/sampling/sampler.h>

#include <cmath>
#include <memory>

/**
 * Direction-space probability distribution for one path continuation event.
 *
 * The random variable is a direction on the unit sphere, a two-dimensional manifold. 
 * 
 * Functions:
 *  - value()    returns probability density with respect to solid angle d_omega
 *  - generate() returns a vector representing a sampled direction.
 *               The vector length has no probabilistic meaning.
 *
 * Implementations must keep both operations consistent: every direction
 * produced by generate() must be evaluated under the same distribution by
 * value(). Renderer uses that density to keep Monte Carlo estimates unbiased.
 */
class PDF {
public:
    virtual ~PDF() = default;

    /** Evaluates directional density in inverse steradians. */
    virtual float value(const Vec3f& direction) const = 0;

    /** Samples a non-zero direction from this distribution. */
    virtual Vec3f generate(Sampler& sampler) const = 0;
};

/** Uniform distribution over the complete unit sphere. */
class SpherePDF : public PDF {
public:
    SpherePDF() {}

    float value([[maybe_unused]] const Vec3f& direction) const override {
        return 1 / (4 * pi);    
    }
    Vec3f generate(Sampler& sampler) const override {
        return sampler.nextUnitVector();
    }
};

/** Cosine-weighted hemisphere distribution around one surface normal. */
class CosineHemispherePDF : public PDF {
public:
    CosineHemispherePDF(const Vec3f& w) :uvw(w) {}

    float value(const Vec3f& direction) const override {
        float cosine_theta = dot(unit_vector(direction), uvw.w());
        return std::fmax(0, cosine_theta / pi);
    }
    Vec3f generate(Sampler& sampler) const override {
        return uvw.transform(sampler.nextCosineHemisphere());
    }
private:
    ONB uvw;
};

/** Direction distribution induced by sampling a Shape from one origin. */
class ShapePDF : public PDF {
public:
    ShapePDF(const Shape& objs, const Point3f& origin)
        : objects(objs), origin(origin) {}

    float value(const Vec3f& direction) const override {
        return objects.getPDFValue(origin, direction);
    }
    Vec3f generate(Sampler& sampler) const override {
        return objects.random(origin, sampler);
    }
private:
    const Shape& objects;
    Point3f origin;
};

/** Equal-probability mixture of two direction distributions. */
class MixturePDF : public PDF {
public:
    MixturePDF(std::shared_ptr<PDF> p0, std::shared_ptr<PDF> p1) {
        p[0] = p0;
        p[1] = p1;
    }

    float value(const Vec3f& direction) const override {
        return 0.5 * p[0]->value(direction) + 0.5 * p[1]->value(direction);
    }
    Vec3f generate(Sampler& sampler) const override {
        if (sampler.next1D() < 0.5f)
            return p[0]->generate(sampler);
        else 
            return p[1]->generate(sampler);
    }

private:
    std::shared_ptr<PDF> p[2];
};

#endif // PDF_H
