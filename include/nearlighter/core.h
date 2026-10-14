#ifndef NEARLIGHTER_CORE_H
#define NEARLIGHTER_CORE_H

/**
 * In-Memory Scene and Rendering Interface
 *
 * Internal headers include precise dependencies rather than this convenience umbrella.
 */

// Base
#include <nearlighter/base/color.h>
#include <nearlighter/base/image.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/base/timer.h>

// Math
#include <nearlighter/math/constants.h>
#include <nearlighter/math/mat4.h>
#include <nearlighter/math/math.h>
#include <nearlighter/math/vec2.h>
#include <nearlighter/math/vec3.h>
#include <nearlighter/math/vec4.h>

// Geometry and Acceleration
#include <nearlighter/accel/bvh.h>
#include <nearlighter/geometry/aabb.h>
#include <nearlighter/geometry/onb.h>
#include <nearlighter/geometry/shading_frame.h>
#include <nearlighter/geometry/transform.h>
#include <nearlighter/shape/all.h>

// Materials and Textures
#include <nearlighter/material/all.h>
#include <nearlighter/texture/all.h>

// Sampling
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampleable_function.h>

// Scene
#include <nearlighter/scene/object/intersectable.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/scene/object/instance.h>
#include <nearlighter/scene/object/linear_aggregate.h>
#include <nearlighter/scene/light/light.h>
#include <nearlighter/scene/light/area_light.h>
#include <nearlighter/scene/light/environment_light.h>
#include <nearlighter/scene/scene.h>

// Rendering
#include <nearlighter/render/hit/hit_record.h>
#include <nearlighter/render/hit/surface_intersection.h>
#include <nearlighter/render/hit/exponential_free_flight.h>
#include <nearlighter/render/camera.h>
#include <nearlighter/render/film.h>
#include <nearlighter/render/integrator.h>
#include <nearlighter/render/light_sampler.h>
#include <nearlighter/render/render_context.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/render/renderer.h>

#endif  // NEARLIGHTER_CORE_H
