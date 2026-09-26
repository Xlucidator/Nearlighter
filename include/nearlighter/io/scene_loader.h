#ifndef NEARLIGHTER_IO_SCENE_LOADER_H
#define NEARLIGHTER_IO_SCENE_LOADER_H

#include <nearlighter/scene/scene.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Material;
class Intersectable;
class Primitive;
class Shape;
class Texture;
class Transform;

/**
 * JSON Scene Loader
 *
 * Loads schema-versioned scene descriptions into runtime Scene objects.
 * - named references are resolved within one file.
 * - relative resource paths use the scene file's directory.
 */
class SceneLoader {
public:
    /**
     * Scene Loading
     *
     * Parses one JSON scene and assembles all declared runtime resources.
     * Unknown fields are ignored.
     *
     * @param scene_path Source file used as the base for relative paths.
     * @return A complete scene ready for rendering.
     * @throws std::runtime_error if the file, schema, data, or a reference is invalid.
     */
    Scene load(const std::filesystem::path& scene_path) const;

private:
    struct LoadContext;

    /* ----- Parsing and Lookup ----- */
    static Vec3f readVector(const std::array<float, 3>& components);
    [[noreturn]] static void fail(const LoadContext& context,
                                  const std::string& reason);
    static std::shared_ptr<Texture> findTexture(
        const LoadContext& context, const std::string& texture_id);

    /* ----- Load Different Part of the Scene ----- */
    /* Independent Scene Values */
    RenderSettings loadRenderSettings(const LoadContext& context) const;
    Camera loadCamera(const LoadContext& context) const;
    Color loadBackground(const LoadContext& context) const;

    /* Referenced Resources */
    void loadTextures(LoadContext& context) const;
    void loadMaterials(LoadContext& context) const;

    /* Objects and Sampling */
    std::shared_ptr<Shape> loadGeometry(
        const LoadContext& context, std::size_t object_index) const;
    Transform loadTransform(const LoadContext& context,
                            std::size_t object_index) const;
    std::shared_ptr<const Intersectable> loadMedium(
        const LoadContext& context, std::size_t object_index,
        std::shared_ptr<const Primitive> boundary) const;
    LinearAggregate loadObjects(LoadContext& context) const;
    LinearAggregate loadGeneratedObjects(const LoadContext& context) const;
    std::vector<std::shared_ptr<const Primitive>> loadSamplingTargets(
        const LoadContext& context) const;
};

#endif  // NEARLIGHTER_IO_SCENE_LOADER_H
