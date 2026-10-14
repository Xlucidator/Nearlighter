#ifndef NEARLIGHTER_MATERIAL_MATERIAL_H
#define NEARLIGHTER_MATERIAL_MATERIAL_H

#include <nearlighter/material/scattering.h>
#include <nearlighter/material/surface/bsdf.h>
#include <nearlighter/material/medium/phase_response.h>
#include <nearlighter/render/hit/hit_record.h>

#include <variant>

/** Material Family for Primitive Construction and Method Compatibility */
enum class MaterialKind { Surface, Medium };

/** Evaluated Surface or Volume Response; monostate Ends Scattering */
using ScatteringResponse = std::variant<std::monostate, BSDF, PhaseResponse>;

/** Shared Material Parameters and Per-Event Response Construction */
class Material {
public:
    virtual ~Material() = default;
    virtual MaterialKind kind() const = 0;

    /**
     * Evaluated Event Scattering
     *
     * - record must belong to this material's family.
     * - Surface events return BSDF; volume events return PhaseResponse.
     * - Resolves textures only after the closest event is known.
     * - Surface responses own evaluated parameters; volume responses borrow the phase.
     * - mode initializes components that need a transport-dependent response.
     * @return BSDF, PhaseResponse, or monostate when no scattering is present.
     * @throws std::bad_variant_access for a mismatched record family.
     */
    virtual ScatteringResponse scattering(
        const HitRecord& record,
        TransportMode mode = TransportMode::Radiance) const = 0;

    /**
     * Directional Event Emission
     *
     * - record must belong to this material's family.
     * - outgoing points away from the event in the record's coordinate space.
     * - Surface emission is radiance; volume emission is a source per unit distance.
     * - A volume source needs distance integration; the current integrator rejects it.
     */
    virtual Color emission(
        [[maybe_unused]] const HitRecord& record,
        [[maybe_unused]] const Vec3f& outgoing) const { return Color(); }

    /** True If Any Supported Point or Direction Can Emit; Independent of Material Family */
    virtual bool isEmissive() const { return false; }
};

#endif  // NEARLIGHTER_MATERIAL_MATERIAL_H
