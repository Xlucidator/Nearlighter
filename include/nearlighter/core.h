#ifndef NEARLIGHTER_CORE_H
#define NEARLIGHTER_CORE_H

/**
 * Complete public interface for in-memory scene construction and rendering.
 *
 * Internal Nearlighter headers use precise dependencies instead of this
 * convenience umbrella.
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

// Materials, Textures, and Media
#include <nearlighter/material/all.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/texture/all.h>

// Sampling
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampler.h>

// Scene and Rendering
#include <nearlighter/render/camera.h>
#include <nearlighter/render/film.h>
#include <nearlighter/render/integrator.h>
#include <nearlighter/light/light.h>
#include <nearlighter/light/area_light.h>
#include <nearlighter/light/environment_light.h>
#include <nearlighter/light/light_sampler.h>
#include <nearlighter/render/render_context.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/instance.h>
#include <nearlighter/scene/intersectable.h>
#include <nearlighter/scene/linear_aggregate.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/scene/scene.h>
#include <nearlighter/scene/surface_interaction.h>

#endif  // NEARLIGHTER_CORE_H
