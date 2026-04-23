#pragma once

#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <array>
#include <optional>
#include <vector>

struct Triangle {
    std::array<Vec3f, 3> pts;

    [[nodiscard]] const Vec3f &operator[](size_t i) const { return pts[i]; }
    Vec3f &operator[](size_t i) { return pts[i]; }
};

struct IShader {
    virtual ~IShader() = default;
    [[nodiscard]] virtual Vec3f vertex(size_t faceIndex, size_t vertIndex) = 0;
    [[nodiscard]] virtual std::optional<TGAColor> fragment(const Vec3f &bary) const = 0;
};

[[maybe_unused]] void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color);

void rasterize(const Triangle &triangle, const IShader &shader,
               std::vector<float> &zbuffer, TGAImage &frameBuffer);