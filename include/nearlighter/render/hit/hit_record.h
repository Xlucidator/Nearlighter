#ifndef NEARLIGHTER_RENDER_HIT_HIT_RECORD_H
#define NEARLIGHTER_RENDER_HIT_HIT_RECORD_H

#include <nearlighter/render/hit/surface_record.h>
#include <nearlighter/render/hit/medium_record.h>

#include <utility>
#include <variant>

class Transform;

/**
 * Surface or Volume Hit Result
 *
 * Value and lifetime contract:
 * - Stores exactly one concrete record without allocating it on the heap.
 * - Uses the queried object's coordinate space.
 * - Default construction alone does not represent a hit.
 * - Material and Primitive access requires bindings from a successful object query.
 */
class HitRecord {
public:
    HitRecord() = default;
    HitRecord(SurfaceRecord record) : value_(std::move(record)) {}
    HitRecord(MediumRecord record) : value_(std::move(record)) {}

    /**
     * @name Concrete Event Access
     * surface() and medium() throw std::bad_variant_access for the other event type.
     * @{
     */
    bool isSurface() const { return std::holds_alternative<SurfaceRecord>(value_); }
    SurfaceRecord& surface() { return std::get<SurfaceRecord>(value_); }
    const SurfaceRecord& surface() const { return std::get<SurfaceRecord>(value_); }
    MediumRecord& medium() { return std::get<MediumRecord>(value_); }
    const MediumRecord& medium() const { return std::get<MediumRecord>(value_); }
    /** @} */

    /**
     * @name Shared Event Data
     * t locates the event along the queried ray; it is independent of Ray::time().
     * @{
     */
    float t() const {
        return std::visit([](const auto& event) { return event.t; }, value_);
    }

    const Point3f& point() const {
        return std::visit([](const auto& event) -> const Point3f& {
            return event.point;
        }, value_);
    }

    const Vec3f& outgoing() const {
        return std::visit([](const auto& event) -> const Vec3f& {
            return event.outgoing;
        }, value_);
    }

    const Material& material() const {
        return std::visit([](const auto& event) -> const Material& {
            return *event.material;
        }, value_);
    }

    const Primitive& primitive() const {
        return std::visit([](const auto& event) -> const Primitive& {
            return *event.primitive;
        }, value_);
    }
    /** @} */

    /** Non-Owning Material and Primitive Binding */
    void bind(const Material& material, const Primitive& primitive) {
        std::visit([&](auto& event) {
            event.material = &material;
            event.primitive = &primitive;
        }, value_);
    }

    /**
     * Completed Event Mapping into Parent Space
     *
     * Mapping contract:
     * - Preserves t and bindings.
     * - Maps position and derives outgoing from the parent ray.
     * - Transforms and face-forwards normals only for surface events.
     */
    void transform(const Transform& local_to_parent, const Ray& parent_ray);

    /**
     * Continuation Ray from the Event
     *
     * - Surface events apply the direction-compatible normal offset.
     * - Volume events use their position directly.
     * @param direction Finite non-zero direction; its length is preserved.
     * @param time Scene time retained by the path.
     */
    Ray spawnRay(const Vec3f& direction, float time) const {
        return std::visit([&](const auto& event) {
            return event.spawnRay(direction, time);
        }, value_);
    }

private:
    std::variant<SurfaceRecord, MediumRecord> value_;
};

#endif  // NEARLIGHTER_RENDER_HIT_HIT_RECORD_H
