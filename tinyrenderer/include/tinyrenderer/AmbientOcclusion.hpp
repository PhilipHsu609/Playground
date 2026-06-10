#pragma once

#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Shader.hpp"
#include "tinyrenderer/Vector.hpp"

#include <array>
#include <cstddef>
#include <random>
#include <vector>

// A seeded unit direction over the upper hemisphere (y >= 0): the "sky"
// directions a surface point's visibility is sampled against.
[[nodiscard]] Vec3f randomHemisphereDir(std::mt19937 &rng);

// Per-camera-pixel surface attributes, for screen-space AO.
struct GBuffer {
    std::vector<Vec3f> worldPos;
    std::vector<Vec3f> normal;
    std::vector<char> covered; // 1 where a fragment was written
    size_t w = 0;
    size_t h = 0;
};

// Rasterize one triangle from the camera, writing interpolated worldPos and
// normal into the GBuffer (and depth into zbuffer) instead of a color. Keyed on
// BlinnPhongShader::Varyings — the only varyings carrying worldPos + normal.
// Mirrors rasterize()'s loop; the shared skeleton is an extraction candidate
// if a third rasterization variant appears.
void captureGBuffer(
    const std::array<VertexOut<BlinnPhongShader::Varyings>, 3> &prim,
    const Mat4f &viewport, std::vector<float> &zbuffer, GBuffer &g);
