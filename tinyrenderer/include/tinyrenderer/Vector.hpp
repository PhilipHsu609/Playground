#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>

template <typename T, size_t N>
class Vec {
    std::array<T, N> data_{};

  public:
    constexpr Vec() = default;

    template <typename... Args>
    constexpr explicit Vec(Args... args) : data_{{args...}} {
        static_assert(sizeof...(args) == N,
                      "Number of arguments must match the dimension of the vector");
    }

    template <typename U, size_t M>
    constexpr explicit Vec(const Vec<U, M> &v) {
        for (size_t i = 0; i < std::min(M, N); ++i) {
            data_[i] = static_cast<T>(v[i]);
        }
    }

    constexpr void clear() { data_.fill(T(0)); }

    template <std::integral U>
    constexpr T &operator[](U i) {
        return data_[static_cast<size_t>(i)];
    }

    template <std::integral U>
    constexpr const T &operator[](U i) const {
        return data_[static_cast<size_t>(i)];
    }

    constexpr Vec operator+(const Vec &v) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] + v[i];
        }
        return result;
    }

    constexpr Vec operator-(const Vec &v) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] - v[i];
        }
        return result;
    }

    constexpr Vec operator*(T s) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] * s;
        }
        return result;
    }

    friend constexpr Vec operator*(T s, const Vec &v) { return v * s; }

    constexpr Vec operator/(T s) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data_[i] / s;
        }
        return result;
    }

    constexpr bool operator==(const Vec &) const = default;

    [[nodiscard]] double norm() const {
        double sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += data_[i] * data_[i];
        }
        return std::sqrt(sum);
    }

    [[nodiscard]] Vec normalize() const
        requires std::floating_point<T>
    {
        const double n = norm();
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = static_cast<T>(data_[i] / n);
        }
        return result;
    }

    [[nodiscard]] double length() const { return norm(); }

    constexpr Vec &operator+=(const Vec &v) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] += v[i];
        }
        return *this;
    }

    constexpr Vec &operator-=(const Vec &v) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] -= v[i];
        }
        return *this;
    }

    constexpr Vec &operator*=(T s) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] *= s;
        }
        return *this;
    }

    constexpr Vec &operator/=(T s) {
        for (size_t i = 0; i < N; ++i) {
            data_[i] /= s;
        }
        return *this;
    }
};

template <typename T, size_t N>
constexpr T dot(const Vec<T, N> &a, const Vec<T, N> &b) {
    T result = 0;
    for (size_t i = 0; i < N; ++i) {
        result += a[i] * b[i];
    }
    return result;
}

template <typename T>
constexpr Vec<T, 3> cross(const Vec<T, 3> &a, const Vec<T, 3> &b) {
    return Vec<T, 3>(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                     a[0] * b[1] - a[1] * b[0]);
}

using Vec4f = Vec<float, 4>;
using Vec3f = Vec<float, 3>;
using Vec2f = Vec<float, 2>;

using Vec4d = Vec<double, 4>;
using Vec3d = Vec<double, 3>;
using Vec2d = Vec<double, 2>;

using Vec4i = Vec<int, 4>;
using Vec3i = Vec<int, 3>;
using Vec2i = Vec<int, 2>;
