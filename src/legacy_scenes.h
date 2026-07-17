#ifndef NEARLIGHTER_APP_LEGACY_SCENES_H
#define NEARLIGHTER_APP_LEGACY_SCENES_H

#include <nearlighter/scene/scene.h>

#include <cstdint>

/**
 * Builds one of the temporary numbered C++ scenes used by the legacy CLI.
 *
 * This interface is application-private and will be removed after JSON scene
 * migration. Scene generation and rendering use independent seeds.
 */
Scene makeLegacyScene(int selection, std::uint64_t generation_seed = 0);

#endif  // NEARLIGHTER_APP_LEGACY_SCENES_H
