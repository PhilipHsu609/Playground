#include "tinyrenderer/AmbientOcclusion.hpp"
#include "tinyrenderer/Shader.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

// Deterministic seeds are intentional: these tests assert reproducible output.
TEST(AmbientOcclusion, HemisphereDirIsUnitLength) {
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::mt19937 rng(123);
    for (int i = 0; i < 50; ++i) {
        const Vec3f d = randomHemisphereDir(rng);
        EXPECT_NEAR(d.norm(), 1.0, 1e-5);
    }
}

TEST(AmbientOcclusion, HemisphereDirPointsUp) {
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::mt19937 rng(123);
    for (int i = 0; i < 50; ++i) {
        const Vec3f d = randomHemisphereDir(rng);
        EXPECT_GE(d.y(), 0.f);
    }
}

TEST(AmbientOcclusion, HemisphereDirIsDeterministic) {
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::mt19937 a(7);
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::mt19937 b(7);
    const Vec3f da = randomHemisphereDir(a);
    const Vec3f db = randomHemisphereDir(b);
    EXPECT_EQ(da, db);
}

TEST(AmbientOcclusion, HemisphereDirAdvancesStream) {
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::mt19937 rng(7);
    const Vec3f first = randomHemisphereDir(rng);
    const Vec3f second = randomHemisphereDir(rng);
    EXPECT_NE(first, second); // each call must consume from the stream
}

// A single triangle facing the camera fills its pixels in the GBuffer with the
// triangle's (constant) world position and leaves the rest uncovered.
TEST(AmbientOcclusion, CaptureGBufferMarksCoverage) {
    constexpr size_t W = 16;
    constexpr size_t H = 16;
    GBuffer g{.worldPos = std::vector<Vec3f>(W * H),
              .normal = std::vector<Vec3f>(W * H),
              .covered = std::vector<char>(W * H, 0),
              .w = W,
              .h = H};
    std::vector<float> zbuffer(W * H, std::numeric_limits<float>::max());

    Mat4f vp;
    vp[0, 0] = W / 2.f;
    vp[1, 1] = H / 2.f;
    vp[2, 2] = 1.f;
    vp[0, 3] = W / 2.f;
    vp[1, 3] = H / 2.f;
    vp[3, 3] = 1.f;

    // Clip-space triangle covering the lower-left quadrant (w = 1).
    using V = BlinnPhongShader::Varyings;
    const V vary{.normal = Vec3f(0.f, 0.f, 1.f), .worldPos = Vec3f(2.f, 3.f, 4.f),
                 .T = Vec3f(), .B = Vec3f(), .uv = Vec2f()};
    const std::array<VertexOut<V>, 3> prim{
        VertexOut<V>{.clip = Vec4f(-1.f, -1.f, 0.f, 1.f), .vary = vary},
        VertexOut<V>{.clip = Vec4f(1.f, -1.f, 0.f, 1.f), .vary = vary},
        VertexOut<V>{.clip = Vec4f(-1.f, 1.f, 0.f, 1.f), .vary = vary},
    };

    captureGBuffer(prim, vp, zbuffer, g);

    // Center of the covered quadrant.
    const size_t idx = static_cast<size_t>(4) * W + 4;
    EXPECT_EQ(g.covered[idx], 1);
    EXPECT_NEAR(g.worldPos[idx].x(), 2.f, 1e-4f);
    // A pixel in the far corner is uncovered.
    EXPECT_EQ(g.covered[W * H - 1], 0);
}

// With no AO buffer bound, AMBIENT * 1 + direct == old AMBIENT + direct, so the
// shader output is unchanged. A fully unlit fragment (light perpendicular to the
// normal) returns exactly the ambient floor on each channel: 255 * 0.1 = 25.
TEST(AmbientOcclusion, NullAoLeavesAmbientFloor) {
    const BlinnPhongShader shader(Model("obj/floor.obj"), Material{},
                                  Mat4f::identity(), Vec3f(0.f, 0.f, 1.f),
                                  Vec3f(0.f, 0.f, 1.f));
    // Normal perpendicular to the light => diffuse 0; no specular; no maps.
    using V = BlinnPhongShader::Varyings;
    const V in{.normal = Vec3f(0.f, 1.f, 0.f), .worldPos = Vec3f(0.f, 0.f, 0.f),
               .T = Vec3f(), .B = Vec3f(), .uv = Vec2f()};
    const TGAColor c = shader.fragment(in, 0, 0);
    EXPECT_EQ(static_cast<int>(c.r), 25); // 255 * AMBIENT(0.1) truncated
}

// With no occluders, no direction is ever blocked, so AO is 1 everywhere it's
// covered. (The depth maps are empty, so inShadow always reports "lit".)
TEST(AmbientOcclusion, BakeAoNoOccludersIsFullyExposed) {
    constexpr size_t W = 8;
    constexpr size_t H = 8;
    const GBuffer g{.worldPos = std::vector<Vec3f>(W * H, Vec3f(0.f, 0.f, 0.f)),
                    .normal = std::vector<Vec3f>(W * H, Vec3f(0.f, 1.f, 0.f)),
                    .covered = std::vector<char>(W * H, 1),
                    .w = W,
                    .h = H};
    Mat4f vp;
    vp[0, 0] = W / 2.f;
    vp[1, 1] = H / 2.f;
    vp[2, 2] = 1.f;
    vp[0, 3] = W / 2.f;
    vp[1, 3] = H / 2.f;
    vp[3, 3] = 1.f;

    const AoLightParams light{.orthoHalf = 2.f, .lightNear = 0.1f, .lightFar = 10.f,
                              .dist = 4.f, .bias = 0.01f};
    const std::vector<float> ao = bakeAO(g, {}, vp, light, 16, 99);

    for (size_t i = 0; i < W * H; ++i) {
        EXPECT_NEAR(ao[i], 1.f, 1e-5f);
    }
}

// A single covered pixel whose camera-depth buffer reports every neighbor at
// the far plane (max depth) has no occluders, so SSAO returns ~1.
TEST(AmbientOcclusion, SsaoOpenSurfaceIsUnoccluded) {
    constexpr size_t W = 8;
    constexpr size_t H = 8;
    const GBuffer g{.worldPos = std::vector<Vec3f>(W * H, Vec3f(0.f, 0.f, 0.f)),
                    .normal = std::vector<Vec3f>(W * H, Vec3f(0.f, 0.f, 1.f)),
                    .covered = std::vector<char>(W * H, 1),
                    .w = W,
                    .h = H};
    // Every stored depth is "far" (max), so any sample is in front => unoccluded.
    const std::vector<float> depth(W * H, std::numeric_limits<float>::max());

    Mat4f vp;
    vp[0, 0] = W / 2.f;
    vp[1, 1] = H / 2.f;
    vp[2, 2] = 1.f;
    vp[0, 3] = W / 2.f;
    vp[1, 3] = H / 2.f;
    vp[3, 3] = 1.f;

    const std::vector<float> ao =
        computeSSAO(g, depth, Mat4f::identity(), vp, 16, 0.5f, 5);

    const size_t idx = static_cast<size_t>(4) * W + 4;
    EXPECT_GT(ao[idx], 0.9f);
}
