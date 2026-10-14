#ifndef NEARLIGHTER_RENDER_RENDER_SETTINGS_H
#define NEARLIGHTER_RENDER_RENDER_SETTINGS_H

#include <nearlighter/render/film.h>

#include <cstdint>

/** Direction Sampling and Direct-Illumination Strategy */
enum class PathSamplingMode { ScatteringOnly, Mixture, LightOnly, MIS };

/** Path Algorithm Settings Shared by Scene Defaults and Render Jobs */
struct PathIntegratorOptions {
    int max_depth = 8;
    // Zero disables roulette; positive values count completed scattering events.
    int russian_roulette_start_depth = 0;
    PathSamplingMode sampling = PathSamplingMode::Mixture;
};

/** Render Job Configuration */
struct RenderSettings {
    // Output dimensions in pixels.
    int image_width = 400;
    int image_height = 400;

    // Beauty is always available; this mask selects additional in-memory layers.
    FilmAOV aovs = FilmAOV::Beauty;

    // Exact number of independent primary paths evaluated for every pixel.
    int samples_per_pixel = 16;

    PathIntegratorOptions path;

    // Root value used to derive every deterministic per-path sequence.
    std::uint64_t seed = 0;
};

#endif  // NEARLIGHTER_RENDER_RENDER_SETTINGS_H
