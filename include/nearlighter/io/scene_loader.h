#ifndef NEARLIGHTER_IO_SCENE_LOADER_H
#define NEARLIGHTER_IO_SCENE_LOADER_H

#include <nearlighter/scene/scene.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

class Material;
class Shape;
class Texture;

/** Loads versioned scene files into complete runtime Scene objects. */
class SceneLoader {
public:
    // ==================================================
    // Public Interface
    // ==================================================

    /**
     * Loads one JSON scene and resolves resources relative to that file.
     *
     * Unknown JSON fields are ignored. Missing required data, unsupported
     * scene elements, and unresolved references throw std::runtime_error.
     */
    Scene load(const std::filesystem::path& scene_path) const;

private:
    /** Temporary source data and reference tables shared by loading stages. */
    struct LoadContext;

    // ==================================================
    // Utility Functions
    // ==================================================

    /** Converts three JSON components into the project vector type. */
    static Vec3f readVector(const std::array<float, 3>& components);

    /** Throws a scene-path-qualified loading error. */
    [[noreturn]] static void fail(const LoadContext& context,
                                  const std::string& reason);

    /** Resolves one named texture or fails at the reference site. */
    static std::shared_ptr<Texture> findTexture(
        const LoadContext& context, const std::string& texture_id);

    // ==================================================
    // Loading Stages
    // ==================================================

    /** Reads reproducible render configuration. */
    RenderSettings loadRenderSettings(const LoadContext& context) const;

    /** Reads perspective camera configuration. */
    Camera loadCamera(const LoadContext& context) const;

    /** Reads the optional constant background. */
    Color loadBackground(const LoadContext& context) const;

    /** Loads optional image textures and resolves their file paths. */
    void loadTextures(LoadContext& context) const;

    /** Constructs named materials after their textures are available. */
    void loadMaterials(LoadContext& context) const;

    /** Constructs one object's base geometry. */
    std::shared_ptr<Shape> loadGeometry(
        const LoadContext& context, std::size_t object_index,
        const std::shared_ptr<Material>& material) const;

    /** Applies one object's transforms in their declared order. */
    std::shared_ptr<Shape> loadTransforms(
        const LoadContext& context, std::size_t object_index,
        std::shared_ptr<Shape> shape) const;

    /** Applies an optional homogeneous medium around one object. */
    std::shared_ptr<Shape> loadMedium(
        const LoadContext& context, std::size_t object_index,
        std::shared_ptr<Shape> boundary) const;

    /** Constructs named scene objects and the world list. */
    ShapeList loadObjects(LoadContext& context) const;

    /** Dispatches the optional built-in procedural scene generator. */
    ShapeList loadGeneratedObjects(const LoadContext& context) const;

    /** Resolves optional importance-sampling object references. */
    ShapeList loadSamplingTargets(const LoadContext& context) const;
};

#endif  // NEARLIGHTER_IO_SCENE_LOADER_H
