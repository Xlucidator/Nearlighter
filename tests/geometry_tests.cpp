#include "test_support.h"

#include <nearlighter/base/interval.h>
#include <nearlighter/base/ray.h>
#include <nearlighter/geometry/aabb.h>
#include <nearlighter/geometry/onb.h>
#include <nearlighter/geometry/transform.h>
#include <nearlighter/material/lambertian.h>
#include <nearlighter/math/constants.h>
#include <nearlighter/math/mat4.h>
#include <nearlighter/math/math.h>
#include <nearlighter/math/vec4.h>
#include <nearlighter/sampling/sampler.h>
#include <nearlighter/scene/primitive.h>
#include <nearlighter/shape/box.h>
#include <nearlighter/shape/mesh.h>
#include <nearlighter/shape/quad.h>
#include <nearlighter/shape/sphere.h>
#include <nearlighter/shape/triangle.h>

#include <cmath>
#include <memory>
#include <stdexcept>

namespace {

constexpr float kTolerance = 1e-5f;

std::shared_ptr<const Material> testMaterial() {
    return std::make_shared<Lambertian>(Color(0.5f, 0.5f, 0.5f));
}

void testVectors(nearlighter::test::Context& context) {
    /* ----- Scalar precision aliases ----- */
    const Vec3d precise(1.0, 2.0, 2.0);
    context.expectTrue(
        std::fabs(precise.length() - 3.0) <= 1e-12,
        "Vec3d should retain double-precision vector arithmetic");
    const Point3d point = precise + Vec3d(2.0, -2.0, 1.0);
    context.expectTrue(
        point.x() == 3.0 && point.y() == 0.0 && point.z() == 3.0,
        "Point3d alias should preserve Vec3d operations");

    /* ----- Four-component operations ----- */
    const Vec4f homogeneous(Vec3f(1.0f, 2.0f, 3.0f), 1.0f);
    const Vec4f scaled = 2 * homogeneous;
    context.expectTrue(
        scaled.x() == 2.0f && scaled.y() == 4.0f &&
            scaled.z() == 6.0f && scaled.w() == 2.0f,
        "Vec4f should support arithmetic scalar multiplication");
    context.expectVecNear(homogeneous.xyz(), Vec3f(1.0f, 2.0f, 3.0f),
                          kTolerance,
                          "Vec4f xyz should recover its first components");
    context.expectNear(dot(homogeneous, homogeneous), 15.0f, kTolerance,
                       "Vec4f dot product should include w");

    /* ----- Direction Mapping ----- */
    context.expectVecNear(
        reflect(Vec3f(1.0f, -1.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f)),
        Vec3f(1.0f, 1.0f, 0.0f), kTolerance,
        "reflection should mirror the normal component");
    context.expectVecNear(
        refract(unit_vector(Vec3f(1.0f, -1.0f, 0.0f)),
                Vec3f(0.0f, 1.0f, 0.0f), 2.0f),
        Vec3f(), kTolerance,
        "refraction should report total internal reflection as zero");
}

void testONB(nearlighter::test::Context& context) {
    const Vec3f direction(1.0f, 0.0f, 1.0f);
    const ONB basis(direction);

    context.expectVecNear(
        basis.w(), unit_vector(direction), kTolerance,
        "ONB w axis should follow its construction direction");
    context.expectNear(dot(basis.u(), basis.v()), 0.0f, kTolerance,
                       "ONB tangent axes should be orthogonal");
    context.expectVecNear(
        cross(basis.u(), basis.v()), basis.w(), kTolerance,
        "ONB axes should form a right-handed basis");

    const Vec3f local(0.25f, -0.5f, 2.0f);
    context.expectVecNear(
        basis.toParent(local),
        ONB(1e-30f * direction).toParent(local), kTolerance,
        "ONB orientation should not depend on direction length");
    context.expectVecNear(
        basis.toParent(Vec3f(0.0f, 0.0f, 1.0f)), basis.w(), kTolerance,
        "ONB should map the local z axis to its parent-space w axis");

    const ONB near_negative_z(Vec3f(1e-4f, -2e-4f, -1.0f));
    context.expectNear(near_negative_z.u().length(), 1.0f, kTolerance,
                       "ONB u axis should remain unit near negative z");
    context.expectNear(near_negative_z.v().length(), 1.0f, kTolerance,
                       "ONB v axis should remain unit near negative z");
    context.expectNear(
        dot(near_negative_z.u(), near_negative_z.w()), 0.0f, kTolerance,
        "ONB u and w axes should remain orthogonal near negative z");
    context.expectNear(
        dot(near_negative_z.v(), near_negative_z.w()), 0.0f, kTolerance,
        "ONB v and w axes should remain orthogonal near negative z");
    context.expectVecNear(
        cross(near_negative_z.u(), near_negative_z.v()),
        near_negative_z.w(), kTolerance,
        "ONB should remain right-handed near negative z");
}

void testSphere(nearlighter::test::Context& context) {
    const Sphere sphere(Point3f(0.0f, 0.0f, -1.0f), 0.5f);
    ShapeHit hit_record;
    const bool hit = sphere.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), hit_record);
    context.expectTrue(hit, "sphere should be hit from outside");
    if (hit) {
        context.expectNear(hit_record.t, 0.5f, kTolerance,
                           "outside sphere hit distance");
        context.expectVecNear(hit_record.point,
                              Point3f(0.0f, 0.0f, -0.5f), kTolerance,
                              "outside sphere local hit point");
        context.expectVecNear(hit_record.geometric_normal,
                              Vec3f(0.0f, 0.0f, 1.0f), kTolerance,
                              "sphere outward geometric normal");
    }

    ShapeHit inside_record;
    const bool inside_hit = sphere.hit(
        Ray(Point3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 0.0f, 0.0f)),
        Interval(0.001f, infinity), inside_record);
    context.expectTrue(inside_hit, "sphere should be hit from inside");
    if (inside_hit) {
        context.expectVecNear(inside_record.geometric_normal,
                              Vec3f(1.0f, 0.0f, 0.0f), kTolerance,
                              "Shape preserves outward normal from inside");
    }

    const Point3f offset_inside_origin(0.25f, 0.0f, -1.0f);
    context.expectNear(
        sphere.getPDFValue(offset_inside_origin,
                           Vec3f(0.0f, 1.0f, 0.0f)),
        1.0f / (4.0f * pi), kTolerance,
        "sphere inside-origin PDF should be uniform over all directions");
}

void testQuad(nearlighter::test::Context& context) {
    const Quad quad(Point3f(-1.0f, -1.0f, -1.0f),
                    Vec3f(2.0f, 0.0f, 0.0f),
                    Vec3f(0.0f, 2.0f, 0.0f));
    ShapeHit record;
    const bool hit = quad.hit(
        Ray(Point3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record);
    context.expectTrue(hit, "quad center ray should hit");
    if (hit) {
        context.expectNear(record.t, 1.0f, kTolerance,
                           "quad hit distance");
        context.expectVecNear(record.geometric_normal,
                              Vec3f(0.0f, 0.0f, 1.0f), kTolerance,
                              "quad geometric normal");
        context.expectNear(record.u, 0.5f, kTolerance,
                           "quad u coordinate");
        context.expectNear(record.v, 0.5f, kTolerance,
                           "quad v coordinate");
    }

    ShapeHit miss_record;
    context.expectFalse(
        quad.hit(Ray(Point3f(2.0f, 0.0f, 0.0f),
                     Vec3f(0.0f, 0.0f, -1.0f)),
                 Interval(0.001f, infinity), miss_record),
        "ray outside quad boundary should miss");
}

void testTriangle(nearlighter::test::Context& context) {
    Sampler sampler(0);
    const Triangle triangle(Point3f(0.0f, 0.0f, -1.0f),
                            Point3f(2.0f, 0.0f, -1.0f),
                            Point3f(0.0f, 2.0f, -1.0f));
    ShapeHit record;
    const bool hit = triangle.hit(
        Ray(Point3f(0.5f, 0.5f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record);
    context.expectTrue(hit, "triangle interior ray should hit");
    if (hit) {
        context.expectNear(record.t, 1.0f, kTolerance,
                           "triangle hit distance");
        context.expectNear(record.u, 0.25f, kTolerance,
                           "triangle first barycentric coordinate");
        context.expectNear(record.v, 0.25f, kTolerance,
                           "triangle second barycentric coordinate");
    }

    ShapeHit back_record;
    const bool back_hit = triangle.hit(
        Ray(Point3f(0.5f, 0.5f, -2.0f),
            Vec3f(0.0f, 0.0f, 1.0f)),
        Interval(0.001f, infinity), back_record);
    context.expectTrue(back_hit, "triangle should intersect from both sides");
    if (back_hit) {
        context.expectVecNear(
            back_record.geometric_normal, Vec3f(0.0f, 0.0f, 1.0f),
            kTolerance,
            "Shape should preserve triangle winding orientation");
    }

    ShapeHit miss_record;
    context.expectFalse(
        triangle.hit(Ray(Point3f(1.5f, 1.5f, 0.0f),
                         Vec3f(0.0f, 0.0f, -1.0f)),
                     Interval(0.001f, infinity), miss_record),
        "ray outside triangle boundary should miss");

    context.expectNear(
        triangle.getPDFValue(Point3f(0.5f, 0.5f, 0.0f),
                             Vec3f(0.0f, 0.0f, -1.0f)),
        0.5f, kTolerance, "triangle area-sampling PDF");
    const Vec3f sampled_direction =
        triangle.random(Point3f(0.0f, 0.0f, 0.0f), sampler);
    context.expectTrue(
        sampled_direction.x() >= 0.0f && sampled_direction.y() >= 0.0f &&
            sampled_direction.x() + sampled_direction.y() <= 2.0f &&
            std::fabs(sampled_direction.z() + 1.0f) <= kTolerance,
        "triangle sample should reach its surface");
}

void testBox(nearlighter::test::Context& context) {
    Sampler sampler(7);
    const Box box(Point3f(-1.0f, -2.0f, -3.0f),
                  Point3f(1.0f, 2.0f, 3.0f));
    ShapeHit record;
    const bool hit = box.hit(
        Ray(Point3f(0.0f, 0.0f, 5.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record);
    context.expectTrue(hit, "box front ray should hit");
    if (hit) {
        context.expectNear(record.t, 2.0f, kTolerance,
                           "box front hit distance");
        context.expectVecNear(record.geometric_normal,
                              Vec3f(0.0f, 0.0f, 1.0f), kTolerance,
                              "box front outward normal");
        context.expectNear(record.u, 0.5f, kTolerance,
                           "box front u coordinate");
        context.expectNear(record.v, 0.5f, kTolerance,
                           "box front v coordinate");
    }

    ShapeHit inside_record;
    context.expectTrue(
        box.hit(Ray(Point3f(0.0f, 0.0f, 0.0f),
                    Vec3f(1.0f, 0.0f, 0.0f)),
                Interval(0.001f, infinity), inside_record),
        "box ray from inside should hit the exit face");
    context.expectVecNear(inside_record.geometric_normal,
                          Vec3f(1.0f, 0.0f, 0.0f), kTolerance,
                          "box inside hit outward normal");

    ShapeHit edge_record;
    context.expectTrue(
        box.hit(Ray(Point3f(1.0f, 0.0f, 5.0f),
                    Vec3f(0.0f, 0.0f, -1.0f)),
                Interval(0.001f, infinity), edge_record),
        "box should accept a parallel ray on a slab boundary");

    const Point3f sampling_origin(0.0f, 0.0f, 5.0f);
    const Vec3f sampling_direction(0.0f, 0.0f, -1.0f);
    const float box_pdf =
        box.getPDFValue(sampling_origin, sampling_direction);
    context.expectNear(box_pdf, 68.0f / 88.0f, kTolerance,
        "box PDF sums near and far surface preimages");
    context.expectNear(
        box.getPDFValue(sampling_origin, 1e-9f * sampling_direction),
        box_pdf, kTolerance,
        "box PDF should not depend on direction length");

    const Vec3f sampled = box.random(sampling_origin, sampler);
    ShapeHit sampled_hit;
    context.expectTrue(
        box.hit(Ray(sampling_origin, sampled),
                Interval(0.001f, infinity), sampled_hit),
        "box sampled direction should reach the surface");
}

void testMesh(nearlighter::test::Context& context) {
    MeshData data;
    data.positions = {
        Point3f(-1.0f, -1.0f, -1.0f),
        Point3f(1.0f, -1.0f, -1.0f),
        Point3f(1.0f, 1.0f, -1.0f),
        Point3f(-1.0f, 1.0f, -1.0f),
    };
    data.texture_coordinates = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
    };
    data.triangles = {{0, 1, 2}, {0, 2, 3}};
    const Mesh mesh(std::move(data), true);

    ShapeHit record;
    const bool hit = mesh.hit(
        Ray(Point3f(0.25f, -0.25f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), record);
    context.expectTrue(hit, "mesh local BVH should hit an indexed triangle");
    if (hit) {
        context.expectVecNear(record.shading_normal,
                              Vec3f(0.0f, 0.0f, 1.0f), kTolerance,
                              "generated mesh shading normal");
        context.expectNear(record.u, 0.625f, kTolerance,
                           "interpolated mesh u coordinate");
        context.expectNear(record.v, 0.375f, kTolerance,
                           "interpolated mesh v coordinate");
    }
}

void testAABB(nearlighter::test::Context& context) {
    const AABB bounds(Point3f(-1.0f, -1.0f, -1.0f),
                      Point3f(1.0f, 1.0f, 1.0f));
    context.expectTrue(
        bounds.hit(
            Ray(Point3f(0.0f, 0.0f, -3.0f), Vec3f(0.0f, 0.0f, 1.0f)),
            Interval(0.0f, infinity)),
        "ray through AABB should hit");
    context.expectFalse(
        bounds.hit(
            Ray(Point3f(2.0f, 0.0f, -3.0f), Vec3f(0.0f, 0.0f, 1.0f)),
            Interval(0.0f, infinity)),
        "parallel ray outside AABB should miss");
    context.expectTrue(
        bounds.hit(
            Ray(Point3f(1.0f, 0.0f, -3.0f),
                Vec3f(0.0f, 0.0f, 1.0f)),
            Interval(0.0f, infinity)),
        "parallel ray on an AABB slab boundary should hit");
}

void testMatrices(nearlighter::test::Context& context) {
    /* ----- Algebraic Operations ----- */
    const Mat4d matrix(
        Vec4d(4.0, 0.0, 0.0, 0.0), Vec4d(7.0, 5.0, 0.0, 0.0),
        Vec4d(2.0, 0.0, 3.0, 0.0), Vec4d(3.0, 1.0, 0.0, 2.0));
    const Vec4d vector(1.0, -2.0, 0.5, 3.0);
    const Vec4d product = matrix * vector;
    context.expectTrue(
        std::fabs(product.x()) <= 1e-12 &&
            std::fabs(product.y() + 7.0) <= 1e-12 &&
            std::fabs(product.z() - 1.5) <= 1e-12 &&
            std::fabs(product.w() - 6.0) <= 1e-12,
        "Mat4d should multiply a column vector");
    context.expectTrue(std::fabs(matrix.determinant() - 120.0) <= 1e-12,
                       "Mat4d should compute a general determinant");
    context.expectTrue(matrix.transposed()(1, 0) == matrix(0, 1),
                       "Mat4d transpose should exchange row and column");

    const Mat4d identity = matrix * matrix.inverse();
    bool inverse_matches = true;
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            const double expected = row == column ? 1.0 : 0.0;
            inverse_matches = inverse_matches &&
                std::fabs(identity(row, column) - expected) <= 1e-12;
        }
    }
    context.expectTrue(inverse_matches,
                       "Mat4d general inverse should restore identity");

    const Mat4d doubled_identity = Mat4d() + Mat4d();
    context.expectTrue(doubled_identity(0, 0) == 2.0 &&
                           doubled_identity(0, 1) == 0.0,
                       "Mat4d addition should operate element-wise");
    const Mat4d masked = hadamard(matrix, Mat4d());
    context.expectTrue(masked(0, 0) == 4.0 && masked(0, 1) == 0.0 &&
                           masked(1, 1) == 5.0,
                       "Mat4d Hadamard product should operate element-wise");

    /* ----- Failure Contract ----- */
    bool singular_rejected = false;
    try {
        (void)Mat4d(0.0).inverse();
    } catch (const std::invalid_argument&) {
        singular_rejected = true;
    }
    context.expectTrue(singular_rejected,
                       "Mat4d inverse should reject a singular matrix");
}

void testPrimitiveTransforms(nearlighter::test::Context& context) {
    Sampler sampler(0);

    /* ----- Matrix Input Contract ----- */
    Mat4f translated_matrix;
    translated_matrix(0, 3) = 2.0f;
    context.expectVecNear(
        Transform(translated_matrix).applyPoint(Point3f(1.0f, 0.0f, 0.0f)),
        Point3f(3.0f, 0.0f, 0.0f), kTolerance,
        "Transform should accept an affine Mat4f");

    Mat4f projective_matrix;
    projective_matrix(3, 0) = 0.5f;
    bool projective_rejected = false;
    try {
        (void)Transform(projective_matrix);
    } catch (const std::invalid_argument&) {
        projective_rejected = true;
    }
    context.expectTrue(projective_rejected,
                       "Transform should reject a projective Mat4f");

    context.expectVecNear(
        Transform::rotate(Vec3f(0.0f, 1.0f, 0.0f), 0.5f * pi)
            .applyVector(Vec3f(1.0f, 0.0f, 0.0f)),
        Vec3f(0.0f, 0.0f, -1.0f), kTolerance,
        "Transform should use right-handed column-vector rotation");

    /* ----- Primitive Mapping ----- */
    auto sphere = std::make_shared<Sphere>(
        Point3f(0.0f, 0.0f, -1.0f), 0.5f);
    const Primitive translated(
        sphere, testMaterial(),
        Transform::translate(Vec3f(2.0f, 0.0f, 0.0f)));

    HitRecord translated_record;
    const bool translated_hit = translated.hit(
        Ray(Point3f(2.0f, 0.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
        Interval(0.001f, infinity), translated_record, sampler);
    context.expectTrue(translated_hit, "translated Primitive should be hit");
    if (translated_hit) {
        context.expectVecNear(translated_record.point,
                              Point3f(2.0f, 0.0f, -0.5f), kTolerance,
                              "translated Primitive world point");
        context.expectTrue(translated_record.front_face,
                           "Primitive computes world front face");
        context.expectTrue(translated_record.material == &translated.material(),
                           "Primitive binds its Material to HitRecord");
    }

    const Transform composed =
        Transform::translate(Vec3f(0.0f, 0.0f, -2.0f)) *
        Transform::scale(Vec3f(2.0f, 1.0f, 1.0f));
    context.expectVecNear(composed.applyPoint(Point3f(1.0f, 0.0f, 0.0f)),
                          Point3f(2.0f, 0.0f, -2.0f), kTolerance,
                          "Transform composition applies right operand first");

    const Vec3f identity_normal =
        unit_vector(Vec3f(1.0f, 2.0f, 3.0f));
    const Vec3f preserved_normal = Transform().applyNormal(identity_normal);
    context.expectTrue(
        preserved_normal.x() == identity_normal.x() &&
            preserved_normal.y() == identity_normal.y() &&
            preserved_normal.z() == identity_normal.z(),
        "identity Transform preserves an existing unit normal exactly");

    context.expectVecNear(
        Transform::scale(Vec3f(2.0f, 1.0f, 1.0f))
            .applyNormal(unit_vector(Vec3f(1.0f, 1.0f, 0.0f))),
        unit_vector(Vec3f(0.5f, 1.0f, 0.0f)), kTolerance,
        "non-uniform scale uses inverse-transpose normal matrix");

    const Primitive stretched(
        std::make_shared<Sphere>(Point3f(0.0f, 0.0f, 0.0f), 1.0f),
        testMaterial(), Transform::scale(Vec3f(2.0f, 1.0f, 1.0f)));
    const Point3f origin(0.0f, 0.0f, 3.0f);
    const Vec3f direction(0.0f, 0.0f, -1.0f);
    const Sphere local_sphere(Point3f(0.0f, 0.0f, 0.0f), 1.0f);
    context.expectNear(
        stretched.getPDFValue(origin, direction),
        0.5f * local_sphere.getPDFValue(origin, direction), kTolerance,
        "non-uniform scale applies solid-angle PDF Jacobian");

    bool singular_rejected = false;
    try {
        const Transform singular = Transform::scale(Vec3f(1.0f, 0.0f, 1.0f));
        (void)singular;
    } catch (const std::invalid_argument&) {
        singular_rejected = true;
    }
    context.expectTrue(singular_rejected,
                       "Transform rejects singular scale");

    const Primitive reflected(
        std::make_shared<Quad>(Point3f(-1.0f, -1.0f, -1.0f),
                               Vec3f(2.0f, 0.0f, 0.0f),
                               Vec3f(0.0f, 2.0f, 0.0f)),
        testMaterial(), Transform::scale(Vec3f(-1.0f, 1.0f, 1.0f)));
    HitRecord reflected_record;
    context.expectTrue(
        reflected.hit(
            Ray(Point3f(0.0f, 0.0f, 0.0f),
                Vec3f(0.0f, 0.0f, -1.0f)),
            Interval(0.001f, infinity), reflected_record, sampler),
        "reflected Primitive should remain intersectable");
    context.expectFalse(reflected_record.front_face,
                        "reflection should reverse oriented surface facing");
}

}  // namespace

int main() {
    nearlighter::test::Context context;
    testVectors(context);
    testONB(context);
    testSphere(context);
    testQuad(context);
    testTriangle(context);
    testBox(context);
    testMesh(context);
    testAABB(context);
    testMatrices(context);
    testPrimitiveTransforms(context);
    return context.finish("geometry tests");
}
