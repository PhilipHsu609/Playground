#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>

template <typename T, size_t N>
class Vec {
    std::array<T, N> data_{};

  public:
    Vec() = default;

    template <typename... Args>
    explicit Vec(Args... args) : data_{{args...}} {
        static_assert(sizeof...(args) == N,
                      "Number of arguments must match the dimension of the vector");
    }

    template <typename U, size_t M>
    explicit Vec(const Vec<U, M> &v) {
        for (size_t i = 0; i < std::min(M, N); ++i) {
            data_[i] = static_cast<T>(v[i]);
        }
    }

    void clear() { data_.fill(T(0)); }

    template <std::integral U>
    T &operator[](U i) {
        return data_[static_cast<size_t>(i)];
    }

    template <std::integral U>
    const T &operator[](U i) const {
        return data_[static_cast<size_t>(i)];
    }

    Vec operator+(const Vec &v) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] + v[i];
        }
        return result;
    }

    Vec operator-(const Vec &v) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] - v[i];
        }
        return result;
    }

    Vec operator*(T s) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] * s;
        }
        return result;
    }

    Vec operator/(T s) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] / s;
        }
        return result;
    }

    bool operator==(const Vec &v) const {
        for (size_t i = 0; i < N; ++i) {
            if (data_[i] != v[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] double norm() const {
        double sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += data_[i] * data_[i];
        }
        return std::sqrt(sum);
    }

    [[nodiscard]] Vec normalize() const {
        const double n = norm();
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = static_cast<T>(data_[i] / n);
        }
        return result;
    }

    [[nodiscard]] double length() const { return norm(); }

    Vec &operator+=(const Vec &v) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] += v[i];
        }
        return *this;
    }

    Vec &operator-=(const Vec &v) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] -= v[i];
        }
        return *this;
    }

    Vec &operator*=(T s) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] *= s;
        }
        return *this;
    }

    Vec &operator/=(T s) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] /= s;
        }
        return *this;
    }

    // Dot product
    T operator*(const Vec &v) const {
        T result = 0;
        for (size_t i = 0; i < N; ++i) {
            result += data_[i] * v[i];
        }
        return result;
    }

    // Vec3 cross product
    Vec operator^(const Vec &v) const
        requires(N == 3)
    {
        return Vec(data_[1] * v[2] - data_[2] * v[1], data_[2] * v[0] - data_[0] * v[2],
                   data_[0] * v[1] - data_[1] * v[0]);
    }
};

using Vec4f = Vec<float, 4>;
using Vec3f = Vec<float, 3>;
using Vec2f = Vec<float, 2>;

using Vec4d = Vec<double, 4>;
using Vec3d = Vec<double, 3>;
using Vec2d = Vec<double, 2>;

using Vec4i = Vec<int, 4>;
using Vec3i = Vec<int, 3>;
using Vec2i = Vec<int, 2>;
