#include <nearlighter/scene/scene.h>

#include <nearlighter/accel/bvh.h>
#include <nearlighter/light/area_light.h>
#include <nearlighter/light/environment_light.h>
#include <nearlighter/material/material.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/scene/instance.h>

#include <stdexcept>
#include <unordered_set>
#include <utility>

/** Temporary caches for shared sources and legacy targets. */
struct Scene::BuildCache {
    struct Source {
        std::shared_ptr<const Intersectable> remainder;
        bool complete = false;
    };
    std::unordered_map<const Intersectable*, Source> source_to_states;
    std::unordered_set<const Material*> materials;
    std::unordered_map<const Primitive*,
                       std::vector<std::shared_ptr<const Primitive>>> primitive_to_placements;
};

// ==================================================
// Scene Construction
//   Emitter extraction: extractEmissivePrimitives()
//   Light creation: buildLights()
// ==================================================

/**
 * @par Implementation
 * Construction order:
 * 1. Collect materials and placed emitters while visiting input objects.
 * 2. Build the final world from non-emissive remainders and placed emitters.
 * 3. Create lights and index them by hit Primitive address.
 */
Scene::Scene(std::string name, Camera camera,
             RenderSettings default_render_settings, Color background,
             LinearAggregate world,
             std::vector<std::shared_ptr<const Primitive>> sampling_targets)
    : name_(std::move(name)), camera_(std::move(camera)),
      default_render_settings_(default_render_settings) {
    using Clock = std::chrono::steady_clock;
    const auto assembly_start = Clock::now();
    BuildCache cache;

    /* ----- Legacy Target Validation ----- */
    for (const auto& target : sampling_targets) {
        if (!target || !target->hasPDF()) {
            throw std::invalid_argument(
                "Scene sampling targets must be sampleable Primitives");
        }
        cache.primitive_to_placements.try_emplace(target.get());
    }

    /* ----- Emissive Primitive Extraction ----- */
    LinearAggregate assembled_world;
    std::vector<std::shared_ptr<const Primitive>> emissive_primitives;
    for (const auto& object : world.objects()) {
        const auto first_emitter = emissive_primitives.size();
        const auto remainder = extractEmissivePrimitives(
            object, Transform(), emissive_primitives, cache);
        if (remainder) assembled_world.add(remainder);
        for (auto index = first_emitter; index < emissive_primitives.size(); ++index) {
            assembled_world.add(emissive_primitives[index]);
        }
    }

    /* ----- Legacy Target Remapping ----- */
    for (const auto& target : sampling_targets) {
        const auto& placements = cache.primitive_to_placements.at(target.get());
        if (placements.empty()) {
            sampling_targets_.push_back(target);
        } else {
            sampling_targets_.insert(sampling_targets_.end(),
                                     placements.begin(), placements.end());
        }
    }
    build_stats_.assembly_time = Clock::now() - assembly_start;

    /* ----- World Acceleration ----- */
    const auto acceleration_start = Clock::now();
    if (assembled_world.size() > 1) {
        world_ = std::make_shared<BVH>(assembled_world);
    } else {
        world_ = std::make_shared<LinearAggregate>(std::move(assembled_world));
    }
    build_stats_.acceleration_build_time = Clock::now() - acceleration_start;

    /* ----- Light Collection ----- */
    const auto light_start = Clock::now();
    buildLights(emissive_primitives, background);
    build_stats_.light_build_time = Clock::now() - light_start;
}

/**
 * @par Implementation
 * Splits each subtree into world-space emitters and a parent-space remainder.
 * - Instance transforms accumulate toward each emissive leaf.
 * - Each emissive placement gets a distinct hit address for MIS lookup.
 * - Unchanged subtrees and cached non-emissive remainders stay shared.
 * - Repeated sources still produce emitters for the current placement.
 * - Unfinished-source revisits indicate a cycle.
 */
std::shared_ptr<const Intersectable> Scene::extractEmissivePrimitives(
    const std::shared_ptr<const Intersectable>& object,
    const Transform& parent_to_world,
    std::vector<std::shared_ptr<const Primitive>>& emissive_primitives,
    BuildCache& cache) {
    /* ----- Shared Source Cache ----- */
    auto [entry, inserted] = cache.source_to_states.try_emplace(object.get());
    auto& source = entry->second;
    if (!inserted) {
        if (!source.complete) {
            throw std::invalid_argument("Scene hierarchy contains a cycle");
        }
        if (source.remainder == object) return object;
    }
    std::shared_ptr<const Intersectable> remainder = object;

    /* ----- Surface or Instance ----- */
    if (const auto primitive = std::dynamic_pointer_cast<const Primitive>(object)) {
        if (inserted && cache.materials.insert(&primitive->material()).second) {
            materials_.push_back(&primitive->material());
        }
        if (primitive->material().isEmissive()) {
            // Distinct placements need distinct hit addresses for the light lookup.
            const std::shared_ptr<const Primitive> placed =
                inserted && parent_to_world.isIdentity()
                ? primitive
                : std::make_shared<Primitive>(primitive->transformed(parent_to_world));
            emissive_primitives.push_back(placed);
            const auto target = cache.primitive_to_placements.find(primitive.get());
            if (target != cache.primitive_to_placements.end()) target->second.push_back(placed);
            remainder.reset();
        }
    } else if (const auto instance = std::dynamic_pointer_cast<const Instance>(object)) {
        const auto child = extractEmissivePrimitives(
            instance->source(), parent_to_world * instance->localToParent(),
            emissive_primitives, cache);
        if (inserted && child != instance->source()) {
            remainder = child
                ? std::make_shared<Instance>(child, instance->localToParent())
                : nullptr;
        }
    } else {
        /* ----- Aggregate or Opaque Content ----- */
        const auto aggregate = std::dynamic_pointer_cast<const LinearAggregate>(object);
        const auto bvh = std::dynamic_pointer_cast<const BVH>(object);
        if (aggregate || bvh) {
            // Read existing BVH leaves without changing its internal nodes.
            const auto snapshot = bvh ? bvh->collectObjects()
                : std::vector<std::shared_ptr<const Intersectable>>();
            const auto& children = aggregate ? aggregate->objects() : snapshot;
            LinearAggregate remaining;
            bool changed = false;
            for (std::size_t index = 0; index < children.size(); ++index) {
                const auto kept = extractEmissivePrimitives(
                    children[index], parent_to_world, emissive_primitives, cache);
                if (!inserted) continue;
                // Allocate a remainder only after the first changed child.
                if (!changed && kept != children[index]) {
                    changed = true;
                    for (std::size_t prefix = 0; prefix < index; ++prefix) {
                        remaining.add(children[prefix]);
                    }
                }
                if (changed && kept) remaining.add(kept);
            }
            if (inserted && changed) {
                if (remaining.empty()) {
                    remainder.reset();
                } else if (bvh && remaining.size() > 1) {
                    remainder = std::make_shared<BVH>(remaining);
                } else {
                    remainder = std::make_shared<LinearAggregate>(std::move(remaining));
                }
            }
        } else if (std::dynamic_pointer_cast<const ConstantMedium>(object)) {
            // Medium boundaries define volume extent, not visible light surfaces.
            contains_media_ = true;
        } else {
            inspection_complete_ = false;
        }
    }

    if (inserted) {
        source.remainder = std::move(remainder);
        source.complete = true;
    }
    return source.remainder;
}

void Scene::buildLights(
    const std::vector<std::shared_ptr<const Primitive>>& emissive_primitives,
    const Color& background) {
    for (const auto& primitive : emissive_primitives) {
        if (!primitive->hasPDF()) {
            ++unsampleable_emitter_count_;
            continue;
        }
        // world_ keeps the Primitive alive after the temporary list is destroyed.
        auto light = std::make_unique<AreaLight>(*primitive);
        primitive_to_arealights_.emplace(primitive.get(), light.get());
        lights_.push_back(std::move(light));
    }
    if (background.x() != 0 || background.y() != 0 || background.z() != 0) {
        auto environment = std::make_unique<ConstantEnvironmentLight>(background);
        environment_ = environment.get();
        lights_.push_back(std::move(environment));
    }
}

// ==================================================
// Surface-to-Light Lookup
// ==================================================

const AreaLight* Scene::findAreaLights(const Primitive& primitive) const {
    const auto light = primitive_to_arealights_.find(&primitive);
    return light == primitive_to_arealights_.end() ? nullptr : light->second;
}
