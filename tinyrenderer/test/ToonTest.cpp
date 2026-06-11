#include "tinyrenderer/Outline.hpp"

#include <gtest/gtest.h>

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
