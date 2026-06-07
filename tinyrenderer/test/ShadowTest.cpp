#include "tinyrenderer/Shader.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {
// 2x2 viewport: NDC [-1,1] -> pixel center [0.5, 1.5] so x=-1 -> 0, x=+1 -> ~1.
Mat4f makeViewport(float w, float h) {
    Mat4f vp;
    vp[0, 0] = w / 2.f;
    vp[1, 1] = h / 2.f;
    vp[2, 2] = 1.f;
    vp[0, 3] = w / 2.f;
    vp[1, 3] = h / 2.f;
    vp[3, 3] = 1.f;
    return vp;
}
} // namespace

TEST(Shadow, OccludedFragmentIsInShadow) {
    const auto identity = Mat4f::identity();
    const auto vp = makeViewport(2.f, 2.f);
    // Stored depth 0.0 at every texel; fragment sits farther (z = 0.5).
    const std::vector<float> shadowMap(4, 0.f);
    const Vec3f worldPos(0.f, 0.f, 0.5f); // -> pixel (1,1), lz = 0.5
    EXPECT_TRUE(inShadow(worldPos, identity, vp, shadowMap, 2, 2, 0.01f));
}

TEST(Shadow, UnoccludedFragmentIsLit) {
    const auto identity = Mat4f::identity();
    const auto vp = makeViewport(2.f, 2.f);
    // Stored depth is the same as the fragment's depth: not occluded (bias saves it).
    const std::vector<float> shadowMap(4, 0.5f);
    const Vec3f worldPos(0.f, 0.f, 0.5f);
    EXPECT_FALSE(inShadow(worldPos, identity, vp, shadowMap, 2, 2, 0.01f));
}

TEST(Shadow, CloserFragmentIsLit) {
    const auto identity = Mat4f::identity();
    const auto vp = makeViewport(2.f, 2.f);
    // Stored occluder is farther (0.9); fragment is nearer the light (0.5) -> lit.
    const std::vector<float> shadowMap(4, 0.9f);
    const Vec3f worldPos(0.f, 0.f, 0.5f);
    EXPECT_FALSE(inShadow(worldPos, identity, vp, shadowMap, 2, 2, 0.01f));
}

TEST(Shadow, OutOfBoundsFragmentIsLit) {
    const auto identity = Mat4f::identity();
    const auto vp = makeViewport(2.f, 2.f);
    const std::vector<float> shadowMap(4, 0.f);
    // x = 5 in NDC projects far outside the 2x2 map -> treated as lit.
    const Vec3f worldPos(5.f, 0.f, 0.5f);
    EXPECT_FALSE(inShadow(worldPos, identity, vp, shadowMap, 2, 2, 0.01f));
}
