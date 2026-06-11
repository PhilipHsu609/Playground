#include "tinyrenderer/Outline.hpp"

#include <gtest/gtest.h>

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
