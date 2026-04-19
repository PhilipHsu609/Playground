#pragma once

#include "Vector.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>

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

    constexpr T &operator()(size_t row, size_t col) { return data_[row * C + col]; }
    constexpr const T &operator()(size_t row, size_t col) const {
        return data_[row * C + col];
    }

    // matrix multiply: Mat<R, C> * Mat<C, K> -> Mat<R, K>
    template <size_t K>
    constexpr Mat<T, R, K> operator*(const Mat<T, C, K> &other) const {
        Mat<T, R, K> result;
        for (size_t i = 0; i < R; ++i) {
            for (size_t j = 0; j < K; ++j) {
                T sum = T(0);
                for (size_t k = 0; k < C; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
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
                sum += (*this)(i, j) * vec[j];
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
            result(i, i) = T(1);
        }
        return result;
    }
};
