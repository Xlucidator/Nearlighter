#include <nearlighter/io/scene_loader.h>

#include <nearlighter/scene/builtin_generator.h>

#include <nearlighter/io/image_io.h>
#include <nearlighter/io/mesh_io.h>
#include <nearlighter/material/dielectric.h>
#include <nearlighter/material/emissive.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/material/metal.h>
#include <nearlighter/math/math.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/geometry/transform.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/shape/box.h>
#include <nearlighter/shape/mesh.h>
#include <nearlighter/shape/quad.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/shape/triangle.h>
#include <nearlighter/texture/checker_texture.h>
#include <nearlighter/texture/image_texture.h>
#include <nearlighter/texture/noise_texture.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

/**
 * Scene Loading Context
 *
 * Owns the parsed document and the named resources resolved from it. The
 * separate sampling table contains only bare surface Primitives.
 */
struct SceneLoader::LoadContext {
    LoadContext(std::filesystem::path path, nlohmann::json source)
        : scene_path(std::move(path)), document(std::move(source)) {}

    std::filesystem::path scene_path;
    nlohmann::json document;

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    std::unordered_map<std::string, std::shared_ptr<const Intersectable>>
        objects;
    std::unordered_map<std::string, std::shared_ptr<const Primitive>>
        sampling_primitives;
};

Vec3f SceneLoader::readVector(const std::array<float, 3>& components) {
    return Vec3f(components[0], components[1], components[2]);
}

void SceneLoader::fail(const LoadContext& context,
                       const std::string& reason) {
    throw std::runtime_error("Failed to load scene '" +
                             context.scene_path.string() + "': " + reason);
}

std::shared_ptr<Texture> SceneLoader::findTexture(
    const LoadContext& context, const std::string& texture_id) {
    const auto texture = context.textures.find(texture_id);
    if (texture == context.textures.end()) {
        fail(context, "unknown texture reference '" + texture_id + "'");
    }
    return texture->second;
}

RenderSettings SceneLoader::loadRenderSettings(
    const LoadContext& context) const {
    const nlohmann::json& render = context.document.at("render");
    const nlohmann::json& resolution = render.at("resolution");
    RenderSettings settings;
    settings.image_width = resolution.at("width").get<int>();
    settings.image_height = resolution.at("height").get<int>();
    settings.samples_per_pixel = render.at("samples_per_pixel").get<int>();
    settings.max_depth = render.at("max_depth").get<int>();
    settings.seed = render.at("seed").get<std::uint64_t>();
    return settings;
}

Camera SceneLoader::loadCamera(const LoadContext& context) const {
    const nlohmann::json& data = context.document.at("camera");
    const std::string type =
        data.value("type", std::string("perspective"));
    if (type != "perspective") {
        fail(context, "unsupported camera type '" + type + "'");
    }

    Camera camera;
    camera.position = readVector(
        data.at("position").get<std::array<float, 3>>());
    camera.look_at = readVector(
        data.at("look_at").get<std::array<float, 3>>());
    camera.vertical_fov = data.at("vertical_fov").get<float>();
    if (data.contains("world_up")) {
        camera.world_up = readVector(
            data.at("world_up").get<std::array<float, 3>>());
    }
    camera.defocus_angle = data.value("defocus_angle", 0.0f);
    camera.focus_distance = data.value("focus_distance", 10.0f);
    return camera;
}

Color SceneLoader::loadBackground(const LoadContext& context) const {
    if (!context.document.contains("background")) {
        return Color(0.0f, 0.0f, 0.0f);
    }
    return readVector(
        context.document.at("background").get<std::array<float, 3>>());
}

void SceneLoader::loadTextures(LoadContext& context) const {
    if (!context.document.contains("textures")) return;

    for (const auto& entry : context.document.at("textures").items()) {
        const std::string& id = entry.key();
        const nlohmann::json& data = entry.value();
        const std::string type = data.at("type").get<std::string>();
        std::shared_ptr<Texture> texture;

        if (type == "image") {
            ImageLoadOptions options;
            const std::string color_space =
                data.value("source_color_space", std::string("srgb"));
            if (color_space == "linear") {
                options.source_color_space = SourceColorSpace::Linear;
            } else if (color_space != "srgb") {
                fail(context, "unsupported texture color space '" +
                                  color_space + "'");
            }

            std::filesystem::path texture_path =
                data.at("path").get<std::string>();
            if (texture_path.is_relative()) {
                texture_path =
                    context.scene_path.parent_path() / texture_path;
            }
            try {
                texture = std::make_shared<ImageTexture>(loadImage(
                    texture_path.lexically_normal(), options));
            } catch (const std::exception& error) {
                fail(context, error.what());
            }
        } else if (type == "checker") {
            const float scale = data.at("scale").get<float>();
            if (scale <= 0.0f) {
                fail(context, "checker texture '" + id +
                                  "' requires a positive scale");
            }
            texture = std::make_shared<CheckerTexture>(
                scale,
                readVector(data.at("even").get<std::array<float, 3>>()),
                readVector(data.at("odd").get<std::array<float, 3>>()));
        } else if (type == "noise") {
            texture = std::make_shared<NoiseTexture>(
                data.at("scale").get<float>(),
                data.value("seed", std::uint64_t{0}));
        } else {
            fail(context, "unsupported texture type '" + type + "'");
        }

        context.textures.emplace(id, std::move(texture));
    }
}

void SceneLoader::loadMaterials(LoadContext& context) const {
    if (!context.document.contains("materials")) return;

    for (const auto& entry : context.document.at("materials").items()) {
        const std::string& id = entry.key();
        const nlohmann::json& data = entry.value();
        const std::string type = data.at("type").get<std::string>();
        std::shared_ptr<Material> material;

        if (type == "lambertian") {
            const bool has_albedo = data.contains("albedo");
            const bool has_texture = data.contains("texture");
            if (has_albedo == has_texture) {
                fail(context,
                     "lambertian material '" + id +
                         "' requires exactly one of albedo or texture");
            }
            if (has_texture) {
                const std::string texture_id =
                    data.at("texture").get<std::string>();
                material = std::make_shared<Lambertian>(
                    findTexture(context, texture_id));
            } else {
                material = std::make_shared<Lambertian>(readVector(
                    data.at("albedo").get<std::array<float, 3>>()));
            }
        } else if (type == "metal") {
            material = std::make_shared<Metal>(
                readVector(data.at("albedo").get<std::array<float, 3>>()),
                data.value("fuzz", 0.0f));
        } else if (type == "dielectric") {
            material = std::make_shared<Dielectric>(
                data.at("refractive_index").get<float>());
        } else if (type == "diffuse_light") {
            const bool has_radiance = data.contains("radiance");
            const bool has_texture = data.contains("texture");
            if (has_radiance == has_texture) {
                fail(context,
                     "diffuse light material '" + id +
                         "' requires exactly one of radiance or texture");
            }
            if (has_texture) {
                const std::string texture_id =
                    data.at("texture").get<std::string>();
                material = std::make_shared<Emissive>(
                    findTexture(context, texture_id));
            } else {
                material = std::make_shared<Emissive>(readVector(
                    data.at("radiance").get<std::array<float, 3>>()));
            }
        } else {
            fail(context, "unsupported material type '" + type + "'");
        }

        context.materials.emplace(id, std::move(material));
    }
}

std::shared_ptr<Shape> SceneLoader::loadGeometry(
    const LoadContext& context, std::size_t object_index) const {
    const nlohmann::json& geometry =
        context.document.at("objects").at(object_index).at("geometry");
    const std::string type = geometry.at("type").get<std::string>();

    if (type == "quad") {
        const Point3f origin = readVector(
            geometry.at("origin").get<std::array<float, 3>>());
        const Vec3f u =
            readVector(geometry.at("u").get<std::array<float, 3>>());
        const Vec3f v =
            readVector(geometry.at("v").get<std::array<float, 3>>());
        if (cross(u, v).near_zero()) {
            fail(context, "quad edges must define a non-zero area");
        }
        return std::make_shared<Quad>(origin, u, v);
    }

    if (type == "box") {
        const Point3f minimum = readVector(
            geometry.at("min").get<std::array<float, 3>>());
        const Point3f maximum = readVector(
            geometry.at("max").get<std::array<float, 3>>());
        if (minimum.x() >= maximum.x() || minimum.y() >= maximum.y() ||
            minimum.z() >= maximum.z()) {
            fail(context, "box max must be greater than min on every axis");
        }
        return std::make_shared<Box>(minimum, maximum);
    }

    if (type == "sphere") {
        const Point3f center = readVector(
            geometry.at("center").get<std::array<float, 3>>());
        const float radius = geometry.at("radius").get<float>();
        if (radius <= 0.0f) {
            fail(context, "sphere radius must be positive");
        }
        return std::make_shared<Sphere>(center, radius);
    }

    if (type == "triangle") {
        const auto vertices = geometry.at("vertices").get<
            std::array<std::array<float, 3>, 3>>();
        return std::make_shared<Triangle>(
            readVector(vertices[0]), readVector(vertices[1]),
            readVector(vertices[2]));
    }

    if (type == "mesh") {
        std::filesystem::path mesh_path =
            geometry.at("path").get<std::string>();
        if (mesh_path.is_relative()) {
            mesh_path = context.scene_path.parent_path() / mesh_path;
        }
        const bool generate_normals =
            geometry.value("generate_normals", false);
        try {
            return std::make_shared<Mesh>(
                loadMeshData(mesh_path.lexically_normal()), generate_normals);
        } catch (const std::exception& error) {
            fail(context, error.what());
        }
    }

    fail(context, "unsupported geometry type '" + type + "'");
}

Transform SceneLoader::loadTransform(
    const LoadContext& context, std::size_t object_index) const {
    const nlohmann::json& object =
        context.document.at("objects").at(object_index);
    Transform result;
    if (!object.contains("transform")) return result;

    try {
        for (const nlohmann::json& transform : object.at("transform")) {
            const std::string type = transform.at("type").get<std::string>();
            Transform operation;
            if (type == "rotate_y") {
                operation = Transform::rotate(
                    Vec3f(0.0f, 1.0f, 0.0f),
                    degrees_to_radians(
                        transform.at("degrees").get<float>()));
            } else if (type == "scale") {
                if (transform.contains("factors")) {
                    operation = Transform::scale(readVector(
                        transform.at("factors").get<
                            std::array<float, 3>>()));
                } else {
                    operation = Transform::scale(
                        transform.at("factor").get<float>());
                }
            } else if (type == "translate") {
                operation = Transform::translate(readVector(
                    transform.at("offset").get<std::array<float, 3>>()));
            } else {
                fail(context, "unsupported transform type '" + type + "'");
            }
            // Pre-multiplication preserves the order declared in the file.
            result = operation * result;
        }
    } catch (const std::invalid_argument& error) {
        fail(context, error.what());
    }
    return result;
}

std::shared_ptr<const Intersectable> SceneLoader::loadMedium(
    const LoadContext& context, std::size_t object_index,
    std::shared_ptr<const Primitive> boundary) const {
    const nlohmann::json& object =
        context.document.at("objects").at(object_index);
    if (!object.contains("medium")) return boundary;

    const nlohmann::json& medium = object.at("medium");
    const float density = medium.at("density").get<float>();
    if (density <= 0.0f) {
        fail(context, "constant medium density must be positive");
    }

    return std::make_shared<ConstantMedium>(
        std::move(boundary), density,
        readVector(medium.at("albedo").get<std::array<float, 3>>()));
}

LinearAggregate SceneLoader::loadObjects(LoadContext& context) const {
    LinearAggregate world;
    if (!context.document.contains("objects")) return world;

    const nlohmann::json& objects = context.document.at("objects");
    for (std::size_t index = 0; index < objects.size(); ++index) {
        const nlohmann::json& data = objects.at(index);
        const std::string id = data.at("id").get<std::string>();
        if (context.objects.count(id) != 0) {
            fail(context, "duplicate object ID '" + id + "'");
        }

        const std::string material_id =
            data.at("material").get<std::string>();
        const auto material = context.materials.find(material_id);
        if (material == context.materials.end()) {
            fail(context, "unknown material reference '" + material_id + "'");
        }

        auto primitive = std::make_shared<Primitive>(
            loadGeometry(context, index), material->second,
            loadTransform(context, index));
        std::shared_ptr<const Intersectable> object =
            loadMedium(context, index, primitive);
        context.objects.emplace(id, object);

        // A medium wrapper has no surface-direction sampling interface.
        if (object.get() == primitive.get()) {
            context.sampling_primitives.emplace(id, primitive);
        }
        world.add(std::move(object));
    }
    return world;
}

LinearAggregate SceneLoader::loadGeneratedObjects(
    const LoadContext& context) const {
    if (!context.document.contains("generator")) return {};

    const nlohmann::json& generator = context.document.at("generator");
    const nlohmann::json& parameters = generator.at("parameters");
    const std::string type = generator.at("type").get<std::string>();

    if (type == "bouncing_spheres") {
        builtin_scenes::BouncingSpheresConfig config;
        config.seed = generator.value("seed", std::uint64_t{0});
        config.grid_size = parameters.value("grid_size", 8);
        config.radius = parameters.value("radius", 0.2f);
        config.spacing = parameters.value("spacing", 0.9f);
        if (config.grid_size <= 0 || config.radius <= 0.0f ||
            config.spacing <= 0.0f) {
            fail(context,
                 "bouncing_spheres parameters must all be positive");
        }
        return builtin_scenes::generateBouncingSpheres(config);
    }

    if (type == "final_scene") {
        builtin_scenes::FinalSceneConfig config;
        config.seed = generator.value("seed", std::uint64_t{0});
        config.ground_grid_size =
            parameters.value("ground_grid_size", 20);
        config.cluster_sphere_count =
            parameters.value("cluster_sphere_count", 1000);
        if (config.ground_grid_size <= 0 ||
            config.cluster_sphere_count <= 0) {
            fail(context, "final_scene counts must be positive");
        }

        const std::string texture_id =
            generator.at("earth_texture").get<std::string>();
        return builtin_scenes::generateFinalScene(
            config, findTexture(context, texture_id));
    }

    fail(context, "unsupported generator type '" + type + "'");
}

std::vector<std::shared_ptr<const Primitive>>
SceneLoader::loadSamplingTargets(
    const LoadContext& context) const {
    std::vector<std::shared_ptr<const Primitive>> sampling_targets;
    if (!context.document.contains("sampling") ||
        !context.document.at("sampling").contains("targets")) {
        return sampling_targets;
    }

    for (const nlohmann::json& target :
         context.document.at("sampling").at("targets")) {
        const std::string id = target.get<std::string>();
        const auto object = context.sampling_primitives.find(id);
        if (object == context.sampling_primitives.end()) {
            if (context.objects.count(id) != 0) {
                fail(context, "sampling target '" + id +
                                  "' is not a surface Primitive");
            }
            fail(context, "unknown sampling target '" + id + "'");
        }
        if (!object->second->hasPDF()) {
            fail(context, "sampling target '" + id +
                              "' does not provide a sampling distribution");
        }
        sampling_targets.push_back(object->second);
    }
    return sampling_targets;
}

/**
 * @par Implementation
 * Loads resources in reference order:
 * - Textures before materials
 * - Materials before objects
 * - Objects before sampling targets
 *
 * JSON exceptions are converted to errors qualified by the source path.
 */
Scene SceneLoader::load(const std::filesystem::path& scene_path) const {
    /* ----- Open Scene File ----- */
    std::ifstream input(scene_path);
    if (!input) {
        throw std::runtime_error("Failed to open scene file '" +
                                 scene_path.string() + "'");
    }

    try {
        /* ----- Parse and Validate ----- */
        LoadContext context(scene_path, nlohmann::json::parse(input));
        if (context.document.at("schema_version").get<int>() != 1) {
            fail(context, "unsupported schema version");
        }

        /* ----- Load Independent Values ----- */
        RenderSettings render_settings = loadRenderSettings(context);
        Camera camera = loadCamera(context);
        Color background = loadBackground(context);

        /* ----- Resolve Referenced Resources ----- */
        loadTextures(context);
        loadMaterials(context);
        LinearAggregate world = loadObjects(context);
        LinearAggregate generated_objects = loadGeneratedObjects(context);
        for (const auto& object : generated_objects.objects()) {
            world.add(object);
        }
        auto sampling_targets = loadSamplingTargets(context);

        /* ----- Assemble Scene ----- */
        return Scene(context.document.at("name").get<std::string>(),
                     std::move(camera), render_settings, background,
                     std::move(world), std::move(sampling_targets));
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error(
            "Failed to load scene '" + scene_path.string() +
            "': invalid or missing JSON data: " + error.what());
    }
}
