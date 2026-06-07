#include "tinyrenderer/Shader.hpp"

#include <gtest/gtest.h>

// Uniform clip-w (orthographic, w == 1 everywhere) leaves barycentrics unchanged.
TEST(PerspectiveCorrect, UniformWIsIdentity) {
    const Vec3f bary(0.25f, 0.5f, 0.25f);
    const Vec3f pc = perspectiveCorrect(bary, Vec3f(1.f, 1.f, 1.f));
    EXPECT_NEAR(pc.x(), 0.25f, 1e-6f);
    EXPECT_NEAR(pc.y(), 0.5f, 1e-6f);
    EXPECT_NEAR(pc.z(), 0.25f, 1e-6f);
}

// At a vertex the weight is entirely on that corner, regardless of w.
TEST(PerspectiveCorrect, VertexStaysAtVertex) {
    const Vec3f pc = perspectiveCorrect(Vec3f(1.f, 0.f, 0.f), Vec3f(3.f, 1.f, 7.f));
    EXPECT_NEAR(pc.x(), 1.f, 1e-6f);
    EXPECT_NEAR(pc.y(), 0.f, 1e-6f);
    EXPECT_NEAR(pc.z(), 0.f, 1e-6f);
}

// The result always renormalizes to sum 1.
TEST(PerspectiveCorrect, WeightsSumToOne) {
    const Vec3f pc = perspectiveCorrect(Vec3f(0.2f, 0.3f, 0.5f), Vec3f(2.f, 4.f, 1.f));
    EXPECT_NEAR(pc.x() + pc.y() + pc.z(), 1.f, 1e-6f);
}

// Correction pulls weight toward the nearer (smaller-w) corner:
// (0.5/1, 0.5/2) normalized = (2/3, 1/3).
TEST(PerspectiveCorrect, FavorsNearerVertex) {
    const Vec3f pc = perspectiveCorrect(Vec3f(0.5f, 0.5f, 0.f), Vec3f(1.f, 2.f, 1.f));
    EXPECT_NEAR(pc.x(), 2.f / 3.f, 1e-6f);
    EXPECT_NEAR(pc.y(), 1.f / 3.f, 1e-6f);
    EXPECT_NEAR(pc.z(), 0.f, 1e-6f);
}

// A constant attribute (T/B, baked equal into all corners) survives blending
// unchanged, while a real varying (uv) blends to its weighted average. This is
// the invariant that lets primitive() compute TBN once per triangle.
TEST(Varyings, ConstantAttributeSurvivesBlend) {
    using V = BlinnPhongShader::Varyings;
    const Vec3f T(0.f, 0.f, 1.f);
    const Vec3f B(0.f, 1.f, 0.f);
    const V a{.normal = Vec3f(1.f, 0.f, 0.f),
              .worldPos = Vec3f(0.f, 0.f, 0.f),
              .T = T,
              .B = B,
              .uv = Vec2f(0.f, 0.f)};
    const V b{.normal = Vec3f(1.f, 0.f, 0.f),
              .worldPos = Vec3f(1.f, 0.f, 0.f),
              .T = T,
              .B = B,
              .uv = Vec2f(1.f, 0.f)};
    const V c{.normal = Vec3f(1.f, 0.f, 0.f),
              .worldPos = Vec3f(0.f, 1.f, 0.f),
              .T = T,
              .B = B,
              .uv = Vec2f(0.f, 1.f)};

    const V blend = a * 0.2f + b * 0.3f + c * 0.5f; // weights sum to 1
    EXPECT_NEAR(blend.T.x(), T.x(), 1e-6f);
    EXPECT_NEAR(blend.T.y(), T.y(), 1e-6f);
    EXPECT_NEAR(blend.T.z(), T.z(), 1e-6f);
    EXPECT_NEAR(blend.B.x(), B.x(), 1e-6f);
    EXPECT_NEAR(blend.B.y(), B.y(), 1e-6f);
    EXPECT_NEAR(blend.B.z(), B.z(), 1e-6f);
    // The uv, a genuine varying, blends to the weighted average.
    EXPECT_NEAR(blend.uv.x(), 0.3f, 1e-6f);
    EXPECT_NEAR(blend.uv.y(), 0.5f, 1e-6f);
}
