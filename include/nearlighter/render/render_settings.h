#ifndef NEARLIGHTER_RENDER_RENDER_SETTINGS_H
#define NEARLIGHTER_RENDER_RENDER_SETTINGS_H

#include <cstdint>

/** Defines the reproducible, machine-independent settings for one render. */
struct RenderSettings {
    // Output dimensions in pixels.
    int image_width = 400;
    int image_height = 400;

    // Exact number of independent primary paths evaluated for every pixel.
    int samples_per_pixel = 16;

    // Maximum number of interactions followed along one path.
    int max_depth = 8;

    // Root value used to derive every deterministic per-path sequence.
    std::uint64_t seed = 0;
};

#endif  // NEARLIGHTER_RENDER_RENDER_SETTINGS_H
