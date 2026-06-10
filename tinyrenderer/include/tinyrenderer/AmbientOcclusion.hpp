#pragma once

#include "tinyrenderer/Drawer.hpp"
#include "tinyrenderer/Matrix.hpp"
#include "tinyrenderer/Model.hpp"
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

// One model bound as an occluder for the AO depth passes.
struct AoOccluder {
    const Model *model;
};

// Light-camera box for the hemisphere depth passes (reuses Lesson 9 values).
struct AoLightParams {
    float orthoHalf;
    float lightNear;
    float lightFar;
    float dist;
    float bias;
};

// Brute-force AO: for each of numDirs hemisphere directions, render a depth map
// and test each covered camera pixel's visibility; average. Returns one value
// per pixel in [0, 1] (1 = fully exposed). Uncovered pixels return 1.
[[nodiscard]] std::vector<float> bakeAO(const GBuffer &g,
                                        const std::vector<AoOccluder> &occluders,
                                        const Mat4f &viewport,
                                        const AoLightParams &light, int numDirs,
                                        unsigned seed);
