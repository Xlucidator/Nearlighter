#include <nearlighter/geometry/transform.h>

#include <cmath>
#include <stdexcept>

namespace {

constexpr float kAffineTolerance = 1e-6f;
constexpr float kSingularTolerance = 1e-8f;

bool isAffine(const Mat4f& matrix) {
    return std::fabs(matrix(3, 0)) <= kAffineTolerance &&
           std::fabs(matrix(3, 1)) <= kAffineTolerance &&
           std::fabs(matrix(3, 2)) <= kAffineTolerance &&
           std::fabs(matrix(3, 3) - 1.0f) <= kAffineTolerance;
}

void validateAffineMatrix(const Mat4f& matrix) {
    if (!matrix.isFinite()) {
        throw std::invalid_argument("Transform matrix entries must be finite");
    }
    if (!isAffine(matrix)) {
        throw std::invalid_argument("Transform matrix must be affine");
    }
}

float linearDeterminant(const Mat4f& matrix) {
    const float a = matrix(0, 0);
    const float b = matrix(0, 1);
    const float c = matrix(0, 2);
    const float d = matrix(1, 0);
    const float e = matrix(1, 1);
    const float f = matrix(1, 2);
    const float g = matrix(2, 0);
    const float h = matrix(2, 1);
    const float i = matrix(2, 2);
    return a * (e * i - f * h) - b * (d * i - f * g) +
           c * (d * h - e * g);
}

/**
 * Affine Matrix Inverse
 *
 * @par Implementation
 * For [A t; 0 1], the inverse is [A^-1 -A^-1 t; 0 1]. The cofactor formula
 * computes only the 3x3 linear inverse required by this structure.
 */
Mat4f inverseAffineMatrix(const Mat4f& matrix) {
    validateAffineMatrix(matrix);

    const float determinant = linearDeterminant(matrix);
    if (!std::isfinite(determinant) ||
        std::fabs(determinant) <= kSingularTolerance) {
        throw std::invalid_argument("Transform matrix must be invertible");
    }

    /* ----- Inverse Linear Block ----- */
    const float a = matrix(0, 0);
    const float b = matrix(0, 1);
    const float c = matrix(0, 2);
    const float d = matrix(1, 0);
    const float e = matrix(1, 1);
    const float f = matrix(1, 2);
    const float g = matrix(2, 0);
    const float h = matrix(2, 1);
    const float i = matrix(2, 2);
    const float inverse_determinant = 1.0f / determinant;

    Mat4f result;
    result(0, 0) = (e * i - f * h) * inverse_determinant;
    result(0, 1) = (c * h - b * i) * inverse_determinant;
    result(0, 2) = (b * f - c * e) * inverse_determinant;
    result(1, 0) = (f * g - d * i) * inverse_determinant;
    result(1, 1) = (a * i - c * g) * inverse_determinant;
    result(1, 2) = (c * d - a * f) * inverse_determinant;
    result(2, 0) = (d * h - e * g) * inverse_determinant;
    result(2, 1) = (b * g - a * h) * inverse_determinant;
    result(2, 2) = (a * e - b * d) * inverse_determinant;

    /* ----- Inverse Translation ----- */
    const Vec3f translation(matrix(0, 3), matrix(1, 3), matrix(2, 3));
    const Vec3f inverse_translation(
        -(result(0, 0) * translation.x() +
          result(0, 1) * translation.y() +
          result(0, 2) * translation.z()),
        -(result(1, 0) * translation.x() +
          result(1, 1) * translation.y() +
          result(1, 2) * translation.z()),
        -(result(2, 0) * translation.x() +
          result(2, 1) * translation.y() +
          result(2, 2) * translation.z()));
    result(0, 3) = inverse_translation.x();
    result(1, 3) = inverse_translation.y();
    result(2, 3) = inverse_translation.z();
    return result;
}

Point3f applyAffinePoint(const Mat4f& matrix, const Point3f& point) {
    return Point3f(
        matrix(0, 0) * point.x() + matrix(0, 1) * point.y() +
            matrix(0, 2) * point.z() + matrix(0, 3),
        matrix(1, 0) * point.x() + matrix(1, 1) * point.y() +
            matrix(1, 2) * point.z() + matrix(1, 3),
        matrix(2, 0) * point.x() + matrix(2, 1) * point.y() +
            matrix(2, 2) * point.z() + matrix(2, 3));
}

Vec3f applyLinearVector(const Mat4f& matrix, const Vec3f& vector) {
    return Vec3f(
        matrix(0, 0) * vector.x() + matrix(0, 1) * vector.y() +
            matrix(0, 2) * vector.z(),
        matrix(1, 0) * vector.x() + matrix(1, 1) * vector.y() +
            matrix(1, 2) * vector.z(),
        matrix(2, 0) * vector.x() + matrix(2, 1) * vector.y() +
            matrix(2, 2) * vector.z());
}

Vec3f applyTransposedLinear(const Mat4f& matrix, const Vec3f& vector) {
    return Vec3f(
        matrix(0, 0) * vector.x() + matrix(1, 0) * vector.y() +
            matrix(2, 0) * vector.z(),
        matrix(0, 1) * vector.x() + matrix(1, 1) * vector.y() +
            matrix(2, 1) * vector.z(),
        matrix(0, 2) * vector.x() + matrix(1, 2) * vector.y() +
            matrix(2, 2) * vector.z());
}

Mat4f translationMatrix(const Vec3f& offset) {
    Mat4f result;
    result(0, 3) = offset.x();
    result(1, 3) = offset.y();
    result(2, 3) = offset.z();
    return result;
}

/**
 * Right-Handed Rotation Matrix
 *
 * @par Implementation
 * Rodrigues' formula expands the axis-angle rotation directly into the
 * upper-left linear block under the column-vector convention.
 */
Mat4f rotationMatrix(const Vec3f& axis, float angle_radians) {
    if (axis.near_zero()) {
        throw std::invalid_argument("Transform rotation axis must be non-zero");
    }

    const Vec3f unit_axis = unit_vector(axis);
    const float x = unit_axis.x();
    const float y = unit_axis.y();
    const float z = unit_axis.z();
    const float cosine = std::cos(angle_radians);
    const float sine = std::sin(angle_radians);
    const float one_minus_cosine = 1.0f - cosine;

    Mat4f result;
    result(0, 0) = cosine + x * x * one_minus_cosine;
    result(0, 1) = x * y * one_minus_cosine - z * sine;
    result(0, 2) = x * z * one_minus_cosine + y * sine;
    result(1, 0) = y * x * one_minus_cosine + z * sine;
    result(1, 1) = cosine + y * y * one_minus_cosine;
    result(1, 2) = y * z * one_minus_cosine - x * sine;
    result(2, 0) = z * x * one_minus_cosine - y * sine;
    result(2, 1) = z * y * one_minus_cosine + x * sine;
    result(2, 2) = cosine + z * z * one_minus_cosine;
    return result;
}

Mat4f scaleMatrix(const Vec3f& factors) {
    Mat4f result;
    result(0, 0) = factors.x();
    result(1, 1) = factors.y();
    result(2, 2) = factors.z();
    return result;
}

}  // namespace

Transform::Transform(const Mat4f& matrix)
    : Transform(matrix, inverseAffineMatrix(matrix)) {}

/**
 * Transform Construction
 *
 * @par Implementation
 * Validates both matrices and caches three independent query results:
 * - Absolute inverse-linear determinant for PDF conversion
 * - Forward orientation reversal
 * - Exact identity state
 */
Transform::Transform(const Mat4f& matrix, const Mat4f& inverse_matrix)
    : matrix_(matrix), inverse_matrix_(inverse_matrix) {
    validateAffineMatrix(matrix_);
    validateAffineMatrix(inverse_matrix_);

    const float determinant = linearDeterminant(matrix_);
    if (!std::isfinite(determinant) ||
        std::fabs(determinant) <= kSingularTolerance) {
        throw std::invalid_argument("Transform matrix must be invertible");
    }

    inverse_linear_abs_determinant_ =
        std::fabs(linearDeterminant(inverse_matrix_));
    is_orientation_reversing_ = determinant < 0.0f;
    is_identity_ = matrix_.isIdentity();
}

Transform Transform::translate(const Vec3f& offset) {
    return Transform(translationMatrix(offset), translationMatrix(-offset));
}

Transform Transform::rotate(const Vec3f& axis, float angle_radians) {
    // Negated angle is the analytic inverse.
    return Transform(rotationMatrix(axis, angle_radians),
                     rotationMatrix(axis, -angle_radians));
}

Transform Transform::scale(const Vec3f& factors) {
    if (std::fabs(factors.x()) <= 1e-8f ||
        std::fabs(factors.y()) <= 1e-8f ||
        std::fabs(factors.z()) <= 1e-8f) {
        throw std::invalid_argument("Transform scale factors must be non-zero");
    }

    const Vec3f inverse(1.0f / factors.x(), 1.0f / factors.y(),
                        1.0f / factors.z());
    return Transform(scaleMatrix(factors), scaleMatrix(inverse));
}

Transform Transform::scale(float factor) {
    return scale(Vec3f(factor, factor, factor));
}

Transform operator*(const Transform& a, const Transform& b) {
    return Transform(a.matrix_ * b.matrix_,
                     b.inverse_matrix_ * a.inverse_matrix_);
}

Point3f Transform::applyPoint(const Point3f& point) const {
    return is_identity_ ? point : applyAffinePoint(matrix_, point);
}

Vec3f Transform::applyVector(const Vec3f& vector) const {
    return is_identity_ ? vector : applyLinearVector(matrix_, vector);
}

/**
 * Normal Transformation
 *
 * @par Implementation
 * For forward linear map A, a normal is transformed by (A^-1)^T.
 * This preserves its orthogonality to every tangent transformed by A.
 * The result is normalized for shading use.
 *
 * The determinant sign remains separate for oriented-surface handling.
 */
Vec3f Transform::applyNormal(const Vec3f& normal) const {
    if (normal.near_zero()) {
        throw std::runtime_error("Transform received a zero-length normal");
    }
    if (is_identity_) return normal;

    const Vec3f transformed = applyTransposedLinear(inverse_matrix_, normal);
    if (transformed.near_zero()) {
        throw std::runtime_error("Transform produced a zero-length normal");
    }
    return unit_vector(transformed);
}

Ray Transform::applyRay(const Ray& ray) const {
    if (is_identity_) return ray;
    return Ray(applyPoint(ray.origin()), applyVector(ray.direction()),
               ray.time());
}

/**
 * Bounds Transformation
 *
 * @par Implementation
 * Transforms all eight corners and encloses them in new axis-aligned extents.
 * Transforming only the original extrema is insufficient after rotation.
 */
AABB Transform::applyBounds(const AABB& bounds) const {
    if (is_identity_) return bounds;

    Point3f minimum(infinity, infinity, infinity);
    Point3f maximum(-infinity, -infinity, -infinity);
    for (int corner_index = 0; corner_index < 8; ++corner_index) {
        const Point3f corner = applyPoint(bounds.corner(corner_index));
        for (int axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::fmin(minimum[axis], corner[axis]);
            maximum[axis] = std::fmax(maximum[axis], corner[axis]);
        }
    }
    return AABB(minimum, maximum);
}

Point3f Transform::applyInversePoint(const Point3f& point) const {
    return is_identity_ ? point : applyAffinePoint(inverse_matrix_, point);
}

Vec3f Transform::applyInverseVector(const Vec3f& vector) const {
    return is_identity_ ? vector : applyLinearVector(inverse_matrix_, vector);
}

Ray Transform::applyInverseRay(const Ray& ray) const {
    if (is_identity_) return ray;

    // Keeping the transformed direction unnormalized preserves parameter t.
    return Ray(applyInversePoint(ray.origin()),
               applyInverseVector(ray.direction()), ray.time());
}

/**
 * @par Implementation
 * Let B be the inverse linear map and w a unit output-space direction:
 * J = d(omega_input) / d(omega_output) = |det(B)| / |B w|^3
 *
 * Normalizing the input first makes the result independent of its length.
 */
float Transform::directionPDFJacobian(
    const Vec3f& output_direction) const {
    const float output_length = output_direction.length();
    if (output_length <= 0.0f) return 0.0f;
    if (is_identity_) return 1.0f;

    const float input_length = applyInverseVector(
        output_direction / output_length).length();
    if (input_length <= 0.0f) return 0.0f;
    return inverse_linear_abs_determinant_ /
           (input_length * input_length * input_length);
}
