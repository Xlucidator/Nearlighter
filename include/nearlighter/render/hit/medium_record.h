#ifndef NEARLIGHTER_RENDER_HIT_MEDIUM_RECORD_H
#define NEARLIGHTER_RENDER_HIT_MEDIUM_RECORD_H

#include <nearlighter/base/ray.h>

class Material;
class Primitive;

/**
 * Volume Event in the Queried Object's Coordinate Space
 *
 * outgoing is unit length; bindings borrow from the hit object's lifetime.
 * Default construction alone does not represent a hit.
 * No surface normal, facing state, or UV exists at a volume event.
 */
struct MediumRecord {
    Point3f point;
    Vec3f outgoing;
    const Material* material = nullptr;
    const Primitive* primitive = nullptr;
    float t = 0.0f;

    /** Continuation from the Event Point; Direction Must Be Finite and Non-Zero */
    Ray spawnRay(const Vec3f& direction, float time) const;
};

#endif  // NEARLIGHTER_RENDER_HIT_MEDIUM_RECORD_H
