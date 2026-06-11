#include "tinyrenderer/Outline.hpp"
#include "tinyrenderer/Shader.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>

TEST(Quantize, ThreeBandsMatchLesson) {
    EXPECT_NEAR(quantize(0.7f, 3), 1.0f, 1e-5f);
    EXPECT_NEAR(quantize(0.5f, 3), 2.f / 3.f, 1e-5f);
    EXPECT_NEAR(quantize(0.2f, 3), 1.f / 3.f, 1e-5f);
}

TEST(Quantize, DarkestBandIsNotBlack) {
    EXPECT_NEAR(quantize(0.0f, 3), 1.f / 3.f, 1e-5f);
}

TEST(Quantize, FullIntensityIsOne) {
    EXPECT_NEAR(quantize(1.0f, 3), 1.0f, 1e-5f);
}

TEST(Quantize, OneBandIsFlat) {
    EXPECT_NEAR(quantize(0.1f, 1), 1.0f, 1e-5f);
    EXPECT_NEAR(quantize(0.9f, 1), 1.0f, 1e-5f);
}

TEST(Quantize, BandBoundaryStraddle) {
    // The 1/3 threshold: just below stays in band 1, just above jumps to band 2.
    EXPECT_NEAR(quantize(1.f / 3.f - 0.01f, 3), 1.f / 3.f, 1e-5f);
    EXPECT_NEAR(quantize(1.f / 3.f + 0.01f, 3), 2.f / 3.f, 1e-5f);
}

TEST(Quantize, MonotonicNonDecreasing) {
    float prev = 0.f;
    for (int i = 0; i <= 100; ++i) {
        const float q = quantize(static_cast<float>(i) / 100.f, 4);
        EXPECT_GE(q, prev);
        prev = q;
    }
}

TEST(SobelMagnitude, FlatBufferIsZero) {
    const std::vector<float> buf(5uz * 5uz, 0.5f);
    const std::vector<float> mag = sobelMagnitude(buf, 5, 5);
    for (const float m : mag) {
        EXPECT_NEAR(m, 0.f, 1e-5f);
    }
}

TEST(SobelMagnitude, VerticalStepIsEdge) {
    // Left half 0, right half 1 -> a vertical edge down the middle column.
    constexpr size_t W = 5;
    constexpr size_t H = 5;
    std::vector<float> buf(W * H, 0.f);
    for (size_t y = 0; y < H; ++y) {
        for (size_t x = 3; x < W; ++x) {
            buf[y * W + x] = 1.f;
        }
    }
    const std::vector<float> mag = sobelMagnitude(buf, W, H);
    // A pixel on the step (interior column 2) has strong magnitude.
    EXPECT_GT(mag[2 * W + 2], 1.f);
    // A pixel in the flat left region (interior column 1) is ~0.
    EXPECT_NEAR(mag[2 * W + 1], 0.f, 1e-5f);
    // Border pixels are 0.
    EXPECT_NEAR(mag[0], 0.f, 1e-5f);
}

TEST(SobelMagnitude, HorizontalStepIsEdge) {
    // Top half 0, bottom half 1 -> a horizontal edge (exercises the Gy kernel,
    // which the vertical-step case leaves at 0).
    constexpr size_t W = 5;
    constexpr size_t H = 5;
    std::vector<float> buf(W * H, 0.f);
    for (size_t y = 3; y < H; ++y) {
        for (size_t x = 0; x < W; ++x) {
            buf[y * W + x] = 1.f;
        }
    }
    const std::vector<float> mag = sobelMagnitude(buf, W, H);
    // A pixel on the step (interior row 2) has strong magnitude.
    EXPECT_GT(mag[2 * W + 2], 1.f);
    // A pixel in the flat top region (interior row 1) is ~0.
    EXPECT_NEAR(mag[1 * W + 2], 0.f, 1e-5f);
}

TEST(ApplyOutline, InksSilhouetteNotInterior) {
    constexpr size_t W = 16;
    constexpr size_t H = 16;
    // A covered block (z = 0) in the middle of a far-background (float::max).
    std::vector<float> depth(W * H, std::numeric_limits<float>::max());
    for (size_t y = 4; y < 12; ++y) {
        for (size_t x = 4; x < 12; ++x) {
            depth[y * W + x] = 0.f;
        }
    }
    TGAImage image(W, H, TGAImage::RGB);
    for (size_t y = 0; y < H; ++y) {
        for (size_t x = 0; x < W; ++x) {
            image.set(static_cast<int>(x), static_cast<int>(y), TGAColor(200, 200, 200));
        }
    }

    applyOutline(image, depth, W, H, 0.1f);

    // A pixel on the block's edge (depth jump in its 3x3) is inked black.
    const TGAColor edge = image.get(4, 7);
    EXPECT_EQ(static_cast<int>(edge.r), 0);
    // A pixel deep inside the block (no jump nearby) keeps its color.
    const TGAColor interior = image.get(7, 7);
    EXPECT_EQ(static_cast<int>(interior.r), 200);
    // A pixel deep in the background keeps its color.
    const TGAColor bg = image.get(0, 0);
    EXPECT_EQ(static_cast<int>(bg.r), 200);
}

TEST(ApplyOutline, NormalizesVaryingCoveredDepths) {
    // Two covered blocks at distinct depths (0.2, 0.8) on a far background, so
    // range > 0 and the (z-min)/range division path runs. The silhouette
    // against the background still inks.
    constexpr size_t W = 16;
    constexpr size_t H = 16;
    std::vector<float> depth(W * H, std::numeric_limits<float>::max());
    for (size_t y = 4; y < 12; ++y) {
        for (size_t x = 4; x < 12; ++x) {
            depth[y * W + x] = x < 8 ? 0.2f : 0.8f;
        }
    }
    TGAImage image(W, H, TGAImage::RGB);
    for (size_t y = 0; y < H; ++y) {
        for (size_t x = 0; x < W; ++x) {
            image.set(static_cast<int>(x), static_cast<int>(y), TGAColor(200, 200, 200));
        }
    }

    applyOutline(image, depth, W, H, 0.1f);

    // The block's outer edge against the background is inked.
    EXPECT_EQ(static_cast<int>(image.get(4, 7).r), 0);
    // A pixel deep in the background keeps its color.
    EXPECT_EQ(static_cast<int>(image.get(0, 0).r), 200);
}

// A fragment lit head-on (n parallel to light) lands in the brightest band, so
// a white-albedo material returns full white. With bands=3 a glancing angle
// (intensity ~0.5) lands in the middle band (2/3).
TEST(ToonShader, QuantizesDiffuseToBands) {
    const ToonShader shader(Model("obj/floor.obj"), Material{}, Mat4f::identity(),
                            Vec3f(0.f, 0.f, 1.f), 3);
    using V = ToonShader::Varyings;
    // Normal facing the light: dot = 1 -> brightest band -> white.
    const V lit{.normal = Vec3f(0.f, 0.f, 1.f), .uv = Vec2f()};
    const TGAColor c = shader.fragment(lit, 0, 0);
    EXPECT_EQ(static_cast<int>(c.r), 255);

    // Normal at ~60 deg to the light: dot = 0.5 -> middle band (2/3) -> ~170.
    const V mid{.normal = Vec3f(std::sqrt(0.75f), 0.f, 0.5f), .uv = Vec2f()};
    const TGAColor cm = shader.fragment(mid, 0, 0);
    EXPECT_EQ(static_cast<int>(cm.r), 170); // 255 * (2/3) truncated

    // Normal facing away: dot < 0 clamps to 0 -> darkest band (1/3), not black.
    const V away{.normal = Vec3f(0.f, 0.f, -1.f), .uv = Vec2f()};
    const TGAColor ca = shader.fragment(away, 0, 0);
    EXPECT_EQ(static_cast<int>(ca.r), 85); // 255 * (1/3) truncated
}
