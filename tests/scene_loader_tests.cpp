#include "test_support.h"

#include <nearlighter/io/scene_loader.h>
#include <nearlighter/material/medium/homogeneous.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/scene/object/primitive.h>
#include <nearlighter/shape/sphere.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

std::string document(const std::string& material, const std::string& object_fields,
                     const std::string& geometry = R"({"type":"sphere","center":[0,0,0],"radius":1})") {
    return R"({"schema_version":1,"name":"loader fixture",
        "render":{"resolution":{"width":4,"height":4},"samples_per_pixel":32,"max_depth":8,"seed":13},
        "camera":{"position":[0,0,0],"look_at":[0,0,-1],"vertical_fov":30},
        "background":[1,1,1],"materials":{"m":)" + material +
        R"(},"objects":[{"id":"object","geometry":)" + geometry + "," + object_fields + "}]}";
}

Scene loadFixture(const std::filesystem::path& path, const std::string& text) {
    { std::ofstream output(path); output << text; }
    return SceneLoader().load(path);
}

void expectRejected(nearlighter::test::Context& context, const std::filesystem::path& path,
                    const std::string& text, const std::string& reason) {
    bool rejected = false;
    try { (void)loadFixture(path, text); }
    catch (const std::runtime_error&) { rejected = true; }
    context.expectTrue(rejected, reason);
}

}  // namespace

int main([[maybe_unused]] int argc, char* argv[]) {
    nearlighter::test::Context context;
    const auto directory = std::filesystem::absolute(argv[0]).parent_path() / "scene_loader_fixture";
    std::filesystem::create_directories(directory);
    const auto path = directory / "scene.json";
    const std::string implicit = R"({"type":"homogeneous","extinction":0.7,"albedo":[0.4,0.6,0.8]})";
    const std::string explicit_phase = R"({"type":"homogeneous","extinction":0.7,"albedo":[0.4,0.6,0.8],"phase":{"type":"isotropic"}})";
    const auto a = loadFixture(path, document(implicit, R"("material":"m")"));
    const auto b = loadFixture(path, document(explicit_phase, R"("material":"m")"));
    context.expectTrue(a.containsMedia() && a.lights().size() == 1,
                       "loaded medium is detected while only the environment becomes a light");
    const auto first = Renderer(a.defaultRenderSettings()).render(a);
    const auto second = Renderer(b.defaultRenderSettings()).render(b);
    const Scene expected("core medium", a.camera(), a.defaultRenderSettings(), Color(1, 1, 1),
        LinearAggregate(std::make_shared<Primitive>(
            std::make_shared<Sphere>(Point3f(), 1),
            std::make_shared<Homogeneous>(0.7f, Color(0.4f, 0.6f, 0.8f)))));
    const auto core = Renderer(expected.defaultRenderSettings()).render(expected);
    context.expectTrue(first.stats.counters.medium_interactions > 0, "loaded material participates in volume transport");
    for (std::size_t i = 0; i < first.image().pixels().size(); ++i) {
        context.expectVecNear(first.image().pixels()[i], core.image().pixels()[i], 0,
                              "JSON geometry and medium parameters match direct core construction");
        context.expectVecNear(first.image().pixels()[i], second.image().pixels()[i], 0,
                              "omitted and explicit isotropic phase produce identical results");
    }
    const auto old = loadFixture(path, document(R"({"type":"lambertian","albedo":[1,1,1]})",
        R"("medium":{"density":0.7,"albedo":[0.4,0.6,0.8]})"));
    const auto migrated = Renderer(old.defaultRenderSettings()).render(old);
    for (std::size_t i = 0; i < first.image().pixels().size(); ++i) {
        context.expectVecNear(first.image().pixels()[i], migrated.image().pixels()[i], 0,
                              "old medium IO mapping needs no dummy surface material");
    }
    expectRejected(context, path, document(implicit,
        R"("material":"m","medium":{"density":1,"albedo":[1,1,1]})"), "duplicate medium definitions are rejected");
    expectRejected(context, path, document(
        R"({"type":"homogeneous","extinction":1,"albedo":[1,1,1],"phase":{"type":"hg"}})",
        R"("material":"m")"), "unimplemented phase types are rejected");
    expectRejected(context, path, document(
        R"({"type":"homogeneous","extinction":1,"albedo":[1,1,1],"phase":{"type":"isotropic","g":0.5}})",
        R"("material":"m")"), "unused phase parameters must not be silently accepted");
    expectRejected(context, path, document(implicit, R"("material":"m")",
        R"({"type":"quad","origin":[0,0,0],"u":[1,0,0],"v":[0,1,0]})"),
        "open geometry cannot define exponential free flight");
    expectRejected(context, path, document(
        R"({"type":"metal","albedo":[0.5,0.5,0.5],"fuzz":0.3})", R"("material":"m")"),
        "removed fuzz parameter requires explicit migration");
    std::filesystem::remove_all(directory);
    return context.finish("Scene loader tests");
}
