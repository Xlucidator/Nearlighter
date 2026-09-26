#include "test_support.h"

#include <nearlighter/accel/bvh.h>
#include <nearlighter/base/interval.h>
#include <nearlighter/light/area_light.h>
#include <nearlighter/material/emissive.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/medium/constant_medium.h>
#include <nearlighter/render/renderer.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/instance.h>
#include <nearlighter/scene/scene.h>
#include <nearlighter/scene/surface_interaction.h>
#include <nearlighter/shape/quad.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/shape/mesh.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

constexpr float kTolerance = 2e-4f;

/** Counts placements rather than unique pointers to detect duplicate geometry. */
int surfaceCount(const Intersectable& object) {
    if (dynamic_cast<const Primitive*>(&object)) return 1;
    if (const auto* instance = dynamic_cast<const Instance*>(&object)) {
        return surfaceCount(*instance->source());
    }
    int count = 0;
    if (const auto* bvh = dynamic_cast<const BVH*>(&object)) {
        for (const auto& child : bvh->collectObjects()) count += surfaceCount(*child);
    } else if (const auto* group = dynamic_cast<const LinearAggregate*>(&object)) {
        for (const auto& child : group->objects()) count += surfaceCount(*child);
    }
    return count;
}

/** Compares nested shared placements with explicitly composed primitives. */
void testInstancedEmitters(nearlighter::test::Context& test) {
    const auto material = std::make_shared<Emissive>(Color(3, 4, 5));
    const auto shape = std::make_shared<Sphere>(Point3f(), 0.5f);
    const Transform local = Transform::translate(Vec3f(0, 1, 0));
    const auto source = std::make_shared<Primitive>(shape, material, local);
    const auto diffuse = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0, -2, 0), 0.5f),
        std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f)));
    LinearAggregate parts;
    parts.add(source);
    parts.add(diffuse);
    const auto shared_bvh = std::make_shared<BVH>(parts);
    const Transform inner = Transform::scale(Vec3f(1.5f, 0.75f, 1));
    const auto shared_instance = std::make_shared<Instance>(shared_bvh, inner);
    const std::array<Transform, 2> outer = {
        Transform::translate(Vec3f(-3, 0, 0)),
        Transform::translate(Vec3f(3, 0, 0))};
    LinearAggregate input;
    for (const auto& transform : outer) {
        input.add(std::make_shared<Instance>(shared_instance, transform));
    }
    Scene scene("instances", Camera(), RenderSettings(), Color(), input, {source});
    test.expectTrue(scene.lights().size() == 2,
                    "each nested placement must produce its own light");
    test.expectTrue(scene.inspectionComplete(), "built-in hierarchy is inspectable");
    test.expectTrue(surfaceCount(scene.world()) == 4,
                    "extracted emitters must replace their old occurrences");
    test.expectTrue(scene.samplingTargets().size() == 2,
                    "legacy source targets should expand to actual emitter placements");
    const auto* final_bvh = dynamic_cast<const BVH*>(&scene.world());
    const Intersectable* residual_source = nullptr;
    int residual_instances = 0;
    for (const auto& object : final_bvh->collectObjects()) {
        if (const auto* instance = dynamic_cast<const Instance*>(object.get())) {
            if (residual_source) {
                test.expectTrue(instance->source().get() == residual_source,
                                "mixed instances must share the same residual source");
            }
            residual_source = instance->source().get();
            ++residual_instances;
        }
    }
    test.expectTrue(residual_instances == 2,
                    "both placements must retain their non-emissive content");

    const Primitive* previous = nullptr;
    for (const auto& transform : outer) {
        const Transform placement = transform * inner * local;
        const Primitive expected(shape, material, placement);
        const Point3f center = placement.applyPoint(Point3f());
        const Ray ray(center + Vec3f(0, 0, 3), Vec3f(0, 0, -1));
        Sampler sampler(12);
        HitRecord actual_hit, expected_hit, input_hit;
        test.expectTrue(scene.world().hit(ray, Interval(0, infinity), actual_hit, sampler),
                        "assembled emitter should remain visible");
        expected.hit(ray, Interval(0, infinity), expected_hit, sampler);
        input.hit(ray, Interval(0, infinity), input_hit, sampler);
        test.expectVecNear(actual_hit.point, expected_hit.point, kTolerance,
                           "placement must include every transform");
        test.expectVecNear(actual_hit.normal, expected_hit.normal, kTolerance,
                           "composed emitter normal must match explicit placement");
        test.expectTrue(input_hit.primitive == source.get(),
                        "assembly must not mutate the shared input subtree");
        test.expectTrue(actual_hit.primitive != previous,
                        "different placements need different hit identities");
        previous = actual_hit.primitive;
        const AreaLight* light = scene.findAreaLights(*actual_hit.primitive);
        test.expectTrue(light && &light->primitive() == actual_hit.primitive,
                        "world hit and light must identify the same primitive");
        if (!light) continue;
        test.expectTrue(&light->primitive().shape() == shape.get() &&
                            &light->primitive().material() == material.get(),
                        "placement must share geometry and material resources");

        const auto receiver = std::make_shared<Primitive>(
            std::make_shared<Sphere>(center + Vec3f(0, 0, 4), 0.5f),
            std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f)));
        const Ray receiver_ray(center + Vec3f(0, 0, 2), Vec3f(0, 0, 1));
        HitRecord receiver_hit;
        receiver->hit(receiver_ray, Interval(0, infinity), receiver_hit, sampler);
        const SurfaceInteraction reference(receiver_ray, receiver_hit);
        for (int sample_index = 0; sample_index < 32; ++sample_index) {
            const auto sample = light->sampleLi(reference, 0, sampler);
            test.expectTrue(sample.has_value(), "placed sphere must be sampleable");
            if (!sample) continue;
            test.expectNear(sample->pdf,
                            expected.getPDFValue(reference.point(), sample->incoming),
                            kTolerance, "light PDF must match the independent placement");
            HitRecord sampled_hit;
            const Ray probe = reference.spawnRay(sample->incoming, 0);
            test.expectTrue(scene.world().hit(probe, Interval(0, infinity),
                                              sampled_hit, sampler),
                            "sampled direction must hit the placed light");
            test.expectTrue(sampled_hit.primitive == actual_hit.primitive,
                            "sampling must not target the untransformed source");
        }
    }

    const auto* root = &scene.world();
    const auto* first_light = scene.lights().front().get();
    Scene moved = std::move(scene);
    test.expectTrue(&moved.world() == root && moved.lights().front().get() == first_light,
                    "moving Scene must preserve world and light addresses");
}

/** A box with separate face materials is a group of six surface primitives. */
void testOneEmissiveFace(nearlighter::test::Context& test) {
    const auto diffuse = std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f));
    const auto emissive = std::make_shared<Emissive>(Color(2, 2, 2));
    auto faces = std::make_shared<LinearAggregate>();
    const std::array<Point3f, 6> origins = {
        Point3f(-1,-1,1), Point3f(1,-1,-1), Point3f(1,-1,1),
        Point3f(-1,-1,-1), Point3f(-1,1,1), Point3f(-1,-1,-1)};
    const std::array<Vec3f, 6> u = {
        Vec3f(2,0,0), Vec3f(-2,0,0), Vec3f(0,0,-2),
        Vec3f(0,0,2), Vec3f(2,0,0), Vec3f(2,0,0)};
    const std::array<Vec3f, 6> v = {
        Vec3f(0,2,0), Vec3f(0,2,0), Vec3f(0,2,0),
        Vec3f(0,2,0), Vec3f(0,0,-2), Vec3f(0,0,2)};
    for (std::size_t index = 0; index < origins.size(); ++index) {
        std::shared_ptr<const Material> material = index == 0
            ? std::static_pointer_cast<const Material>(emissive)
            : std::static_pointer_cast<const Material>(diffuse);
        faces->add(std::make_shared<Primitive>(
            std::make_shared<Quad>(origins[index], u[index], v[index]), material));
    }
    LinearAggregate world;
    world.add(std::make_shared<Instance>(faces, Transform::translate(Vec3f(0, 0, -3))));
    const Scene scene("one face", Camera(), RenderSettings(), Color(), world);
    test.expectTrue(surfaceCount(scene.world()) == 6,
                    "assembly must preserve exactly six box faces");
    test.expectTrue(scene.lights().size() == 1,
                    "only the emissive face should become a light");
    for (int side : {-1, 1}) {
        Sampler sampler(0);
        HitRecord hit;
        const Ray ray(Point3f(0, 0, -3 + side * 4), Vec3f(0, 0, -side));
        test.expectTrue(scene.world().hit(ray, Interval(0, infinity), hit, sampler),
                        "both emissive and non-emissive faces must remain visible");
        test.expectTrue((scene.findAreaLights(*hit.primitive) != nullptr) == (side == 1),
                        "light association must follow the face material");
    }
}

void testMediumBoundary(nearlighter::test::Context& test) {
    const auto boundary = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 1),
        std::make_shared<Emissive>(Color(2, 2, 2)));
    LinearAggregate world;
    world.add(std::make_shared<ConstantMedium>(boundary, 1, Color(1, 1, 1)));
    const Scene scene("medium", Camera(), RenderSettings(), Color(), world);
    test.expectTrue(scene.containsMedia(), "medium content must remain visible to validation");
    test.expectTrue(scene.lights().empty(), "a hidden medium boundary is not a surface light");
}

/** Equivalent explicit and instanced scenes must feed the same MIS estimator. */
void testNestedRender(nearlighter::test::Context& test) {
    const auto shape = std::make_shared<Sphere>(Point3f(), 0.5f);
    const auto emission = std::make_shared<Emissive>(Color(4, 4, 4));
    const Transform local = Transform::translate(Vec3f(0, 3, 0));
    const auto source = std::make_shared<Primitive>(shape, emission, local);
    const auto group = std::make_shared<BVH>(LinearAggregate(source));
    const Transform inner = Transform::scale(Vec3f(1.5f, 0.75f, 1));
    const auto inner_instance = std::make_shared<Instance>(group, inner);
    const auto receiver = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 1),
        std::make_shared<Lambertian>(Color(0.7f, 0.7f, 0.7f)));
    LinearAggregate nested(receiver), explicit_world(receiver);
    for (float x : {-2.0f, 2.0f}) {
        const Transform outer = Transform::translate(Vec3f(x, 0, 0));
        nested.add(std::make_shared<Instance>(inner_instance, outer));
        explicit_world.add(std::make_shared<Primitive>(shape, emission, outer * inner * local));
    }
    Camera camera;
    camera.position = Point3f(0, 0, 6);
    camera.look_at = Point3f();
    camera.vertical_fov = 40;
    RenderSettings settings;
    settings.image_width = 12;
    settings.image_height = 12;
    settings.samples_per_pixel = 16;
    settings.max_depth = 4;
    const Scene instanced("nested", camera, settings, Color(), nested);
    const Scene expanded("expanded", camera, settings, Color(), explicit_world);
    const auto* world = &instanced.world();
    const auto* light = instanced.lights().front().get();
    for (auto mode : {DirectLightingMode::BSDFOnly, DirectLightingMode::LightOnly,
                      DirectLightingMode::MIS}) {
        RenderOptions options;
        options.integrator = IntegratorKind::Path;
        options.path.direct_lighting = mode;
        const auto actual = Renderer(settings, options).render(instanced);
        const auto expected = Renderer(settings, options).render(expanded);
        bool nonzero = false;
        for (int y = 0; y < settings.image_height; ++y) {
            for (int x = 0; x < settings.image_width; ++x) {
                test.expectVecNear(actual.image().at(x, y), expected.image().at(x, y),
                                   1e-5f, "nested emitter render must match explicit placements");
                nonzero |= !actual.image().at(x, y).near_zero();
            }
        }
        test.expectTrue(nonzero, "equivalence must exercise nonzero transport");
        test.expectTrue(actual.stats.invalid_contributions == 0 &&
                            actual.stats.invalid_pdf_terminations == 0,
                        "nested emitters must not generate invalid transport events");
    }
    settings.image_width = 6;
    settings.image_height = 4;
    RenderOptions options;
    options.integrator = IntegratorKind::Path;
    const auto resized = Renderer(settings, options).render(instanced);
    test.expectTrue(resized.image().width() == 6 && resized.image().height() == 4,
                    "one assembled Scene must support a different render size");
    test.expectTrue(&instanced.world() == world && instanced.lights().front().get() == light,
                    "rendering must reuse scene acceleration and lights");
}

/** A source may appear both through an instance and as a direct scene member. */
void testDirectAndInstancedEmitter(nearlighter::test::Context& test) {
    const auto source = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(0, 0, -3), 0.5f),
        std::make_shared<Emissive>(Color(2, 2, 2)));
    LinearAggregate world(std::make_shared<Instance>(
        source, Transform::translate(Vec3f(3, 0, 0))));
    world.add(source);
    const Scene scene("direct and instanced", Camera(), RenderSettings(),
                      Color(), world, {source});
    test.expectTrue(scene.lights().size() == 2 && scene.samplingTargets().size() == 2,
                    "both placements must enter lights and legacy proposals");
    const Primitive* previous = nullptr;
    Sampler sampler(0);
    for (float x : {3.0f, 0.0f}) {
        HitRecord hit;
        const bool found = scene.world().hit(
            Ray(Point3f(x, 0, 0), Vec3f(0, 0, -1)), Interval(0, infinity), hit, sampler);
        test.expectTrue(found, "both the instance and the direct emitter must remain visible");
        if (!found) continue;
        const auto* light = scene.findAreaLights(*hit.primitive);
        test.expectTrue(hit.primitive != previous && light &&
                            &light->primitive() == hit.primitive,
                        "each placement must retain its own light association");
        previous = hit.primitive;
    }
}

void testSharedNonEmissiveSubtree(nearlighter::test::Context& test) {
    LinearAggregate parts;
    for (float x : {-1.0f, 1.0f}) {
        parts.add(std::make_shared<Primitive>(
            std::make_shared<Sphere>(Point3f(x, 0, 0), 0.5f),
            std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f))));
    }
    const auto shared = std::make_shared<BVH>(parts);
    const auto instance = std::make_shared<Instance>(shared, Transform::translate(Vec3f(0,0,-3)));
    const Scene scene("shared", Camera(), RenderSettings(), Color(), LinearAggregate(instance));
    const auto* root = dynamic_cast<const LinearAggregate*>(&scene.world());
    test.expectTrue(root && root->objects().front().get() == instance.get(),
                    "a non-emissive subtree must retain its original instance and BVH");
}

void testUnsampleableEmitter(nearlighter::test::Context& test) {
    MeshData data;
    data.positions = {Point3f(0,0,0), Point3f(1,0,0), Point3f(0,1,0)};
    data.triangles = {{0, 1, 2}};
    const auto mesh = std::make_shared<Mesh>(std::move(data));
    const auto emitter = std::make_shared<Primitive>(
        mesh, std::make_shared<Emissive>(Color(2,2,2)));
    const auto instance = std::make_shared<Instance>(emitter, Transform::translate(Vec3f(0,0,-2)));
    LinearAggregate world(instance);
    world.add(std::make_shared<Instance>(emitter, Transform::translate(Vec3f(3,0,-2))));
    const Scene scene("mesh emitter", Camera(), RenderSettings(), Color(), world);
    test.expectTrue(scene.unsampleableEmitterCount() == 2 && scene.lights().empty(),
                    "validation must count unsampleable placements rather than sources");
    test.expectTrue(surfaceCount(scene.world()) == 2,
                    "unsampleable emitters must remain owned by the visibility world");
    bool rejected = false;
    try { PathIntegrator::validateScene(scene); }
    catch (const std::invalid_argument&) { rejected = true; }
    test.expectTrue(rejected, "path validation must reject an unsampleable nested emitter");
}

/** A custom query wrapper exposes no inspectable scene structure. */
class OpaqueObject final : public Intersectable {
public:
    explicit OpaqueObject(std::shared_ptr<const Primitive> primitive)
        : primitive_(std::move(primitive)) {}
    bool hit(const Ray& ray, Interval ray_t, HitRecord& record,
             Sampler& sampler) const override {
        return primitive_->hit(ray, ray_t, record, sampler);
    }
    const AABB& getBoundingBox() const override { return primitive_->getBoundingBox(); }
private:
    std::shared_ptr<const Primitive> primitive_;
};

void testOpaqueContent(nearlighter::test::Context& test) {
    const auto primitive = std::make_shared<Primitive>(
        std::make_shared<Sphere>(Point3f(), 1),
        std::make_shared<Emissive>(Color(2,2,2)));
    const Scene scene("opaque", Camera(), RenderSettings(), Color(),
                      LinearAggregate(std::make_shared<OpaqueObject>(primitive)));
    test.expectFalse(scene.inspectionComplete(),
                     "opaque content must not be certified as having no hidden lights");
    bool rejected = false;
    try { PathIntegrator::validateScene(scene); }
    catch (const std::invalid_argument&) { rejected = true; }
    test.expectTrue(rejected, "path validation must reject unknown content");
    Sampler sampler(0);
    HitRecord record;
    test.expectTrue(scene.world().hit(Ray(Point3f(0,0,3), Vec3f(0,0,-1)),
                                      Interval(0, infinity), record, sampler),
                    "unknown content must remain available to legacy intersection");
}

}  // namespace

int main() {
    nearlighter::test::Context test;
    testInstancedEmitters(test);
    testDirectAndInstancedEmitter(test);
    testOneEmissiveFace(test);
    testMediumBoundary(test);
    testNestedRender(test);
    testSharedNonEmissiveSubtree(test);
    testUnsampleableEmitter(test);
    testOpaqueContent(test);
    return test.finish("scene tests");
}
