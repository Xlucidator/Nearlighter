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
#include <nearlighter/base/onb.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/base/timer.h>

// Math
#include <nearlighter/math/constants.h>
#include <nearlighter/math/math.h>
#include <nearlighter/math/vec3f.h>

// Geometry and Acceleration
#include <nearlighter/accel/bvh_node.h>
#include <nearlighter/geometry/all.h>

// Materials, Textures, and Media
#include <nearlighter/material/all.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/texture/all.h>

// Sampling
#include <nearlighter/sampling/pdf.h>
#include <nearlighter/sampling/sampler.h>

// Scene and Rendering
#include <nearlighter/render/camera.h>
#include <nearlighter/render/render_settings.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/scene.h>

// Transforms
#include <nearlighter/transform/all.h>

#endif  // NEARLIGHTER_CORE_H
