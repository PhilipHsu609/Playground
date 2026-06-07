#pragma once

#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/TGAImage.hpp"
#include "tinyrenderer/Vector.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <vector>

// A varyings payload: any vector space the rasterizer can interpolate. Shaders
// define their own struct (normal, uv, ...) satisfying this.
template <typename V>
concept Blendable = requires(V v, float s) {
    { v * s } -> std::same_as<V>;
    { v + v } -> std::same_as<V>;
};

// A shader's output for one triangle corner. `clip` is clip-space, BEFORE the
// perspective divide (the rasterizer owns the divide).
template <Blendable V>
struct VertexOut {
    Vec4f clip;
    V vary;
};

// A shader the rasterizer can drive: primitive(face) returns the three corners,
// fragment(vary) shades a pixel.
template <typename S>
concept Shader = Blendable<typename S::Varyings> &&
                 requires(const S s, size_t f, const typename S::Varyings v) {
                     { s.faceCount() } -> std::convertible_to<size_t>;
                     {
                         s.primitive(f)
                     } -> std::same_as<std::array<VertexOut<typename S::Varyings>, 3>>;
                     { s.fragment(v) } -> std::same_as<TGAColor>;
                 };

[[nodiscard]] Vec3f barycentric(Vec2f p, Vec2f t0, Vec2f t1, Vec2f t2);

// Reweight screen-space barycentrics by 1/w so attributes interpolate in clip
// space, not screen space (without this, textures warp under perspective).
// Depth is exempt: NDC z is already correct in screen space.
[[nodiscard]] Vec3f perspectiveCorrect(const Vec3f &bary, const Vec3f &clipW);

[[maybe_unused]] void line(Vec2i u, Vec2i v, TGAImage &image, TGAColor color);

// Rasterize one triangle: perspective divide, viewport, depth test, and
// perspective-correct shading via shader.fragment().
//
// `viewport` must leave z untouched, so the depth stored here stays comparable
// to the raw clip.z() that inShadow() reads back.
template <Shader S>
void rasterize(const S &shader,
               const std::array<VertexOut<typename S::Varyings>, 3> &prim,
               const Mat4f &viewport, std::vector<float> &zbuffer,
               TGAImage &frameBuffer) {
    using V = typename S::Varyings;

    std::array<Vec3f, 3> screen;
    Vec3f clipW;
    for (size_t i = 0; i < 3; ++i) {
        const float w = prim[i].clip.w();
        clipW[i] = w;
        screen[i] = Vec3f(viewport * (prim[i].clip / w));
    }

    const Vec2f s0(screen[0]);
    const Vec2f s1(screen[1]);
    const Vec2f s2(screen[2]);

    const float minXf = std::min({s0.x(), s1.x(), s2.x()});
    const float maxXf = std::max({s0.x(), s1.x(), s2.x()});
    const float minYf = std::min({s0.y(), s1.y(), s2.y()});
    const float maxYf = std::max({s0.y(), s1.y(), s2.y()});

    const int minX = std::max(static_cast<int>(std::floor(minXf)), 0);
    const int maxX = std::min(static_cast<int>(std::ceil(maxXf)),
                              static_cast<int>(frameBuffer.getWidth()) - 1);
    const int minY = std::max(static_cast<int>(std::floor(minYf)), 0);
    const int maxY = std::min(static_cast<int>(std::ceil(maxYf)),
                              static_cast<int>(frameBuffer.getHeight()) - 1);

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            const Vec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const Vec3f bary = barycentric(p, s0, s1, s2);
            if (bary.x() < 0 || bary.y() < 0 || bary.z() < 0) {
                continue;
            }

            // Depth uses the raw barycentric: NDC z is linear in screen space.
            const float z = bary.x() * screen[0].z() + bary.y() * screen[1].z() +
                            bary.z() * screen[2].z();
            const auto index =
                static_cast<size_t>(y) * frameBuffer.getWidth() + static_cast<size_t>(x);
            if (zbuffer[index] <= z) {
                continue;
            }

            const Vec3f pc = perspectiveCorrect(bary, clipW);
            const V vary =
                prim[0].vary * pc.x() + prim[1].vary * pc.y() + prim[2].vary * pc.z();

            zbuffer[index] = z;
            frameBuffer.set(x, y, shader.fragment(vary));
        }
    }
}
