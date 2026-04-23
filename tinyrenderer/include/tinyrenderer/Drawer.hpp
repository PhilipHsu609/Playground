#pragma once

#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <array>
#include <vector>

struct Triangle {
    std::array<Vec3f, 3> pts;

    [[nodiscard]] const Vec3f &operator[](size_t i) const { return pts[i]; }
    Vec3f &operator[](size_t i) { return pts[i]; }
};

[[maybe_unused]] void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color);

void rasterize(const Triangle &triangle, std::vector<float> &zbuffer,
               TGAImage &frameBuffer, TGAColor color);