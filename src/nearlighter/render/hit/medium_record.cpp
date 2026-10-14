#include <nearlighter/render/hit/medium_record.h>

#include <cmath>
#include <stdexcept>

Ray MediumRecord::spawnRay(const Vec3f& direction, float time) const {
    if (!std::isfinite(direction.x()) || !std::isfinite(direction.y()) ||
        !std::isfinite(direction.z()) || direction.near_zero()) {
        throw std::invalid_argument("Medium ray direction must be finite and non-zero");
    }
    // A volume event has no geometric boundary requiring an offset.
    return Ray(point, direction, time);
}
