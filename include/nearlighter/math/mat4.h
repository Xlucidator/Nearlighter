#ifndef NEARLIGHTER_MATH_MAT4_H
#define NEARLIGHTER_MATH_MAT4_H

#include <nearlighter/math/vec4.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

/**
 * Four-by-Four Floating-Point Matrix
 *
 * - Supports float and double scalar types.
 * - Stores elements in column-major order.
 * - Multiplies column vectors on the right.
 *
 * The type represents an arbitrary 4x4 matrix. It assigns no point, direction,
 * normal, or coordinate-space semantics to its operands.
 */
template <typename T>
class Mat4 {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
                  "Mat4 supports only float and double scalars");

public:
    /** Creates a diagonal matrix; the default value creates identity. */
    explicit constexpr Mat4(T diagonal = T(1)) : elements_{} {
        (*this)(0, 0) = diagonal;
        (*this)(1, 1) = diagonal;
        (*this)(2, 2) = diagonal;
        (*this)(3, 3) = diagonal;
    }

    /** Creates a matrix from four column vectors. */
    constexpr Mat4(const Vec4<T>& column0, const Vec4<T>& column1,
                   const Vec4<T>& column2, const Vec4<T>& column3)
        : elements_{column0.x(), column0.y(), column0.z(), column0.w(),
                    column1.x(), column1.y(), column1.z(), column1.w(),
                    column2.x(), column2.y(), column2.z(), column2.w(),
                    column3.x(), column3.y(), column3.z(), column3.w()} {}

    /**
     * @name Element Access
     * Uses mathematical row-column indices over contiguous column-major
     * storage. Index validity is the caller's responsibility.
     * @{
     */
    constexpr T operator()(std::size_t row, std::size_t column) const {
        return elements_[index(row, column)];
    }
    constexpr T& operator()(std::size_t row, std::size_t column) {
        return elements_[index(row, column)];
    }
    constexpr const T* data() const { return elements_.data(); }
    constexpr T* data() { return elements_.data(); }
    /** @} */

    constexpr Mat4 operator-() const {
        Mat4 result(T(0));
        for (std::size_t i = 0; i < elements_.size(); ++i) {
            result.elements_[i] = -elements_[i];
        }
        return result;
    }

    /** @name In-Place Algebraic Operations
     * @{ */
    constexpr Mat4& operator+=(const Mat4& other) {
        for (std::size_t i = 0; i < elements_.size(); ++i) {
            elements_[i] += other.elements_[i];
        }
        return *this;
    }

    constexpr Mat4& operator-=(const Mat4& other) {
        for (std::size_t i = 0; i < elements_.size(); ++i) {
            elements_[i] -= other.elements_[i];
        }
        return *this;
    }

    constexpr Mat4& operator*=(T scalar) {
        for (T& element : elements_) element *= scalar;
        return *this;
    }

    constexpr Mat4& operator/=(T scalar) {
        return *this *= T(1) / scalar;
    }

    constexpr Mat4& operator*=(const Mat4& other);
    /** @} */

    /** Returns the transposed matrix. */
    constexpr Mat4 transposed() const {
        Mat4 result(T(0));
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result(column, row) = (*this)(row, column);
            }
        }
        return result;
    }

    /** Returns the matrix determinant. */
    T determinant() const;

    /**
     * Matrix Inverse
     *
     * @return The inverse of this matrix.
     * @throws std::invalid_argument if an entry is non-finite or the matrix is
     * numerically singular.
     */
    Mat4 inverse() const;

    /** Reports whether every matrix entry is finite. */
    bool isFinite() const {
        for (const T element : elements_) {
            if (!std::isfinite(element)) return false;
        }
        return true;
    }

    /** Reports exact identity. */
    constexpr bool isIdentity() const {
        for (std::size_t column = 0; column < 4; ++column) {
            for (std::size_t row = 0; row < 4; ++row) {
                const T expected = row == column ? T(1) : T(0);
                if ((*this)(row, column) != expected) return false;
            }
        }
        return true;
    }

private:
    static constexpr std::size_t index(std::size_t row,
                                       std::size_t column) {
        return column * 4 + row;
    }

    std::array<T, 16> elements_;
};

using Mat4f = Mat4<float>;
using Mat4d = Mat4<double>;

template <typename T>
constexpr Mat4<T> operator+(Mat4<T> left, const Mat4<T>& right) {
    return left += right;
}

template <typename T>
constexpr Mat4<T> operator-(Mat4<T> left, const Mat4<T>& right) {
    return left -= right;
}

/**
 * Matrix Product
 *
 * Satisfies (a * b) * v = a * (b * v) under the column-vector convention.
 */
template <typename T>
constexpr Mat4<T> operator*(const Mat4<T>& a, const Mat4<T>& b) {
    Mat4<T> result(T(0));
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            T element = T(0);
            for (std::size_t inner = 0; inner < 4; ++inner) {
                element += a(row, inner) * b(inner, column);
            }
            result(row, column) = element;
        }
    }
    return result;
}

/** Multiplies a matrix by a column vector. */
template <typename T>
constexpr Vec4<T> operator*(const Mat4<T>& matrix, const Vec4<T>& vector) {
    Vec4<T> result;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            result[row] += matrix(row, column) * vector[column];
        }
    }
    return result;
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Mat4<T> operator*(Mat4<T> matrix, U scalar) {
    return matrix *= static_cast<T>(scalar);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Mat4<T> operator*(U scalar, Mat4<T> matrix) {
    return matrix *= static_cast<T>(scalar);
}

template <typename T, typename U,
          std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
constexpr Mat4<T> operator/(Mat4<T> matrix, U scalar) {
    return matrix /= static_cast<T>(scalar);
}

/** Returns the element-wise matrix product. */
template <typename T>
constexpr Mat4<T> hadamard(const Mat4<T>& a, const Mat4<T>& b) {
    Mat4<T> result(T(0));
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            result(row, column) = a(row, column) * b(row, column);
        }
    }
    return result;
}

/** Reports exact element-wise equality. */
template <typename T>
constexpr bool operator==(const Mat4<T>& a, const Mat4<T>& b) {
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            if (a(row, column) != b(row, column)) return false;
        }
    }
    return true;
}

template <typename T>
constexpr bool operator!=(const Mat4<T>& a, const Mat4<T>& b) {
    return !(a == b);
}

template <typename T>
constexpr Mat4<T>& Mat4<T>::operator*=(const Mat4& other) {
    return *this = *this * other;
}

extern template class Mat4<float>;
extern template class Mat4<double>;

#endif  // NEARLIGHTER_MATH_MAT4_H
