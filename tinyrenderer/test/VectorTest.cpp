#include "tinyrenderer/Vector.hpp"

#include <fmt/core.h>
#include <gtest/gtest.h>

TEST(Vector, Operators) {
    Vec3f v1(1.0f, 2.0f, 3.0f);
    Vec3f v2(4.0f, 5.0f, 6.0f);

    // Test addition
    Vec3f v3 = v1 + v2;
    EXPECT_TRUE(v3 == Vec3f(5.0f, 7.0f, 9.0f));

    // Test subtraction
    v3 = v1 - v2;
    EXPECT_TRUE(v3 == Vec3f(-3.0f, -3.0f, -3.0f));

    // Test scalar multiplication
    v3 = v1 * 2.0f;
    EXPECT_TRUE(v3 == Vec3f(2.0f, 4.0f, 6.0f));

    // Test scalar division
    v3 = v1 / 2.0f;
    EXPECT_TRUE(v3 == Vec3f(0.5f, 1.0f, 1.5f));

    // Test dot product
    float dot = v1 * v2;
    EXPECT_EQ(dot, 32.0f);

    // Test cross product
    v3 = v1 ^ v2;
    EXPECT_TRUE(v3 == Vec3f(-3.0f, 6.0f, -3.0f));

    // Test normalization
    Vec3d v4 = v1.normalize();
    float norm = std::sqrt(1.0f + 4.0f + 9.0f);
    EXPECT_NEAR(v4[0], 1.0f / norm, 1e-6);
    EXPECT_NEAR(v4[1], 2.0f / norm, 1e-6);
    EXPECT_NEAR(v4[2], 3.0f / norm, 1e-6);

    // Test in-place addition
    v3.clear();
    v3 += v2;
    EXPECT_TRUE(v3 == Vec3f(4.0f, 5.0f, 6.0f));

    // Test in-place subtraction
    v3.clear();
    v3 -= v2;
    EXPECT_TRUE(v3 == Vec3f(-4.0f, -5.0f, -6.0f));

    // Test in-place scalar multiplication
    v3 *= 2.0f;
    EXPECT_TRUE(v3 == Vec3f(-8.0f, -10.0f, -12.0f));

    // Test in-place scalar division
    v3 /= 2.0f;
    EXPECT_TRUE(v3 == Vec3f(-4.0f, -5.0f, -6.0f));

    // Test type conversion
    Vec3i v5(v3);
    EXPECT_TRUE(v5 == Vec3i(-4, -5, -6));

    // Test vector size conversion
    Vec2f v6(v3);
    EXPECT_TRUE(v6 == Vec2f(-4.0f, -5.0f));
    Vec3f v7(v6);
    EXPECT_TRUE(v7 == Vec3f(-4.0f, -5.0f, 0.0f));
}