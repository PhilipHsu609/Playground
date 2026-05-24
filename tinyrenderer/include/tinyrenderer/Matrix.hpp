#pragma once

#include "Vector.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <initializer_list>
#include <stdexcept>

enum class MatrixError {
    SINGULAR,
};

template <typename T, size_t R, size_t C>
class Mat {
    std::array<T, R * C> data_{};

  public:
    constexpr Mat() = default;

    constexpr Mat(std::initializer_list<std::initializer_list<T>> rows) {
        if (rows.size() != R) {
            throw std::invalid_argument("Mat: row count mismatch");
        }
        size_t i = 0;
        for (const auto &row : rows) {
            if (row.size() != C) {
                throw std::invalid_argument("Mat: column count mismatch");
            }
            size_t j = 0;
            for (const auto &val : row) {
                data_[i * C + j] = val;
                ++j;
            }
            ++i;
        }
    }

    [[nodiscard]] constexpr auto &operator[](this auto &self, size_t row, size_t col) {
        return self.data_[row * C + col];
    }

    // matrix multiply: Mat<R, C> * Mat<C, K> -> Mat<R, K>
    template <size_t K>
    constexpr Mat<T, R, K> operator*(const Mat<T, C, K> &other) const {
        Mat<T, R, K> result;
        for (size_t i = 0; i < R; ++i) {
            for (size_t j = 0; j < K; ++j) {
                T sum = T(0);
                for (size_t k = 0; k < C; ++k) {
                    sum += (*this)[i, k] * other[k, j];
                }
                result[i, j] = sum;
            }
        }
        return result;
    }

    // matrix-vector multiply: Mat<R, C> * Vec<C> -> Vec<R>
    constexpr Vec<T, R> operator*(const Vec<T, C> &vec) const {
        Vec<T, R> result;
        for (size_t i = 0; i < R; ++i) {
            T sum = T(0);
            for (size_t j = 0; j < C; ++j) {
                sum += (*this)[i, j] * vec[j];
            }
            result[i] = sum;
        }
        return result;
    }

    static constexpr Mat identity()
        requires(R == C)
    {
        Mat result;
        for (size_t i = 0; i < R; ++i) {
            result[i, i] = T(1);
        }
        return result;
    }

    [[nodiscard]] constexpr Mat<T, C, R> transpose() const {
        Mat<T, C, R> result;
        for (size_t i = 0; i < R; ++i) {
            for (size_t j = 0; j < C; ++j) {
                result[j, i] = (*this)[i, j];
            }
        }
        return result;
    }

    // Closed-form 2x2 inverse:  [[a b]; [c d]]^-1 = (1/(ad-bc)) * [[d -b]; [-c a]].
    // Returns MatrixError::Singular when det == 0 so callers must decide how
    // to handle degenerate input (skip, fall back, propagate) rather than
    // silently producing inf/nan.
    [[nodiscard]] constexpr std::expected<Mat, MatrixError> inverse() const
        requires(R == 2 && C == 2 && std::floating_point<T>)
    {
        const T det = (*this)[0, 0] * (*this)[1, 1] - (*this)[0, 1] * (*this)[1, 0];
        if (det == T(0)) {
            return std::unexpected(MatrixError::SINGULAR);
        }
        const T invDet = T(1) / det;
        return Mat{
            {(*this)[1, 1] * invDet, -(*this)[0, 1] * invDet},
            {-(*this)[1, 0] * invDet, (*this)[0, 0] * invDet},
        };
    }
};

using Mat3f = Mat<float, 3, 3>;
using Mat4f = Mat<float, 4, 4>;
