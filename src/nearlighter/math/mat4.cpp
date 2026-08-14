#include <nearlighter/math/mat4.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

/**
 * @par Implementation
 * Triangular elimination uses row pivoting. Each row swap changes the sign;
 * the diagonal product of the resulting upper triangle gives the magnitude.
 */
template <typename T>
T Mat4<T>::determinant() const {
    std::array<std::array<T, 4>, 4> work{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            work[row][column] = (*this)(row, column);
        }
    }

    T result = T(1);
    for (std::size_t pivot_column = 0; pivot_column < 4; ++pivot_column) {
        std::size_t pivot_row = pivot_column;
        T pivot_magnitude = std::fabs(work[pivot_row][pivot_column]);
        for (std::size_t row = pivot_column + 1; row < 4; ++row) {
            const T candidate = std::fabs(work[row][pivot_column]);
            if (candidate > pivot_magnitude) {
                pivot_row = row;
                pivot_magnitude = candidate;
            }
        }
        if (pivot_magnitude == T(0)) return T(0);

        if (pivot_row != pivot_column) {
            std::swap(work[pivot_row], work[pivot_column]);
            result = -result;
        }

        const T pivot = work[pivot_column][pivot_column];
        result *= pivot;
        for (std::size_t row = pivot_column + 1; row < 4; ++row) {
            const T factor = work[row][pivot_column] / pivot;
            for (std::size_t column = pivot_column + 1; column < 4;
                 ++column) {
                work[row][column] -= factor * work[pivot_column][column];
            }
        }
    }
    return result;
}

/**
 * @par Implementation
 * Gauss-Jordan elimination reduces [M | I] to [I | M^-1]. Scaled partial
 * pivoting compares candidates relative to their rows, so uniformly small but
 * well-conditioned matrices remain invertible.
 */
template <typename T>
Mat4<T> Mat4<T>::inverse() const {
    if (!isFinite()) {
        throw std::invalid_argument("Mat4 entries must be finite");
    }

    /* ----- Augmented System ----- */
    std::array<std::array<T, 8>, 4> augmented{};
    std::array<T, 4> row_scales{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            const T element = (*this)(row, column);
            augmented[row][column] = element;
            row_scales[row] = std::fmax(row_scales[row], std::fabs(element));
        }
        augmented[row][row + 4] = T(1);
    }

    /* ----- Scaled Pivot Elimination ----- */
    constexpr T relative_tolerance =
        T(64) * std::numeric_limits<T>::epsilon();
    for (std::size_t pivot_column = 0; pivot_column < 4; ++pivot_column) {
        std::size_t pivot_row = pivot_column;
        T best_ratio = T(0);
        for (std::size_t row = pivot_column; row < 4; ++row) {
            if (row_scales[row] == T(0)) continue;
            const T ratio =
                std::fabs(augmented[row][pivot_column]) / row_scales[row];
            if (ratio > best_ratio) {
                pivot_row = row;
                best_ratio = ratio;
            }
        }
        if (best_ratio <= relative_tolerance) {
            throw std::invalid_argument("Mat4 must be invertible");
        }

        if (pivot_row != pivot_column) {
            std::swap(augmented[pivot_row], augmented[pivot_column]);
            std::swap(row_scales[pivot_row], row_scales[pivot_column]);
        }

        const T pivot = augmented[pivot_column][pivot_column];
        for (T& element : augmented[pivot_column]) element /= pivot;

        for (std::size_t row = 0; row < 4; ++row) {
            if (row == pivot_column) continue;
            const T factor = augmented[row][pivot_column];
            if (factor == T(0)) continue;
            for (std::size_t column = 0; column < 8; ++column) {
                augmented[row][column] -=
                    factor * augmented[pivot_column][column];
            }
        }
    }

    /* ----- Inverse Extraction ----- */
    Mat4 result(T(0));
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            result(row, column) = augmented[row][column + 4];
        }
    }
    if (!result.isFinite()) {
        throw std::invalid_argument("Mat4 inverse must be finite");
    }
    return result;
}

template class Mat4<float>;
template class Mat4<double>;
