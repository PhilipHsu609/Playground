#include "tinyrenderer/AmbientOcclusion.hpp"

#include <gtest/gtest.h>

#include <cmath>

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
