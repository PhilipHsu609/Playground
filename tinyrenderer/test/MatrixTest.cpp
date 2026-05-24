#include "tinyrenderer/Matrix.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(Mat, NestedListConstruction) {
    Mat<float, 2, 3> m{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}};
    EXPECT_EQ((m[0, 0]), 1.f);
    EXPECT_EQ((m[0, 1]), 2.f);
    EXPECT_EQ((m[0, 2]), 3.f);
    EXPECT_EQ((m[1, 0]), 4.f);
    EXPECT_EQ((m[1, 1]), 5.f);
    EXPECT_EQ((m[1, 2]), 6.f);
}

TEST(Mat, IdentityIsDiagonal) {
    auto m = Mat<float, 3, 3>::identity();
    EXPECT_EQ((m[0, 0]), 1.f);
    EXPECT_EQ((m[1, 1]), 1.f);
    EXPECT_EQ((m[2, 2]), 1.f);
    EXPECT_EQ((m[0, 1]), 0.f);
    EXPECT_EQ((m[1, 0]), 0.f);
}

TEST(Mat, MatrixMultiplyIdentity) {
    const Mat<float, 2, 2> m{{1.f, 2.f}, {3.f, 4.f}};
    const auto id = Mat<float, 2, 2>::identity();
    const auto result = m * id;
    EXPECT_EQ((result[0, 0]), 1.f);
    EXPECT_EQ((result[0, 1]), 2.f);
    EXPECT_EQ((result[1, 0]), 3.f);
    EXPECT_EQ((result[1, 1]), 4.f);
}

TEST(Mat, MatrixMultiplyNonSquare) {
    // 2x3 * 3x2 = 2x2
    const Mat<float, 2, 3> a{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}};
    const Mat<float, 3, 2> b{{7.f, 8.f}, {9.f, 10.f}, {11.f, 12.f}};
    const auto c = a * b;
    EXPECT_EQ((c[0, 0]), 58.f);  // 1*7 + 2*9 + 3*11
    EXPECT_EQ((c[0, 1]), 64.f);  // 1*8 + 2*10 + 3*12
    EXPECT_EQ((c[1, 0]), 139.f); // 4*7 + 5*9 + 6*11
    EXPECT_EQ((c[1, 1]), 154.f); // 4*8 + 5*10 + 6*12
}

TEST(Mat, MatrixVectorMultiplyIdentity) {
    const auto m = Mat<float, 3, 3>::identity();
    const Vec3f v(1.f, 2.f, 3.f);
    const Vec3f result = m * v;
    EXPECT_EQ(result, v);
}

TEST(Mat, MatrixVectorMultiply) {
    // rotation by 90 deg around z-axis: x -> y, y -> -x
    const Mat<float, 3, 3> m{{0.f, -1.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}};
    const Vec3f v(1.f, 0.f, 0.f);
    const Vec3f result = m * v;
    EXPECT_NEAR(result.x(), 0.f, 1e-6);
    EXPECT_NEAR(result.y(), 1.f, 1e-6);
    EXPECT_NEAR(result.z(), 0.f, 1e-6);
}

TEST(Mat, ConstRead) {
    const Mat<float, 2, 2> m{{1.f, 2.f}, {3.f, 4.f}};
    EXPECT_EQ((m[0, 0]), 1.f);
    EXPECT_EQ((m[1, 1]), 4.f);
}

TEST(Mat, WrongRowCountThrows) {
    EXPECT_THROW((Mat<float, 2, 2>{{1.f, 2.f}}), std::invalid_argument);
    EXPECT_THROW((Mat<float, 2, 2>{{1.f, 2.f}, {3.f, 4.f}, {5.f, 6.f}}),
                 std::invalid_argument);
}

TEST(Mat, WrongColumnCountThrows) {
    EXPECT_THROW((Mat<float, 2, 2>{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}}),
                 std::invalid_argument);
    EXPECT_THROW((Mat<float, 2, 2>{{1.f}, {2.f}}), std::invalid_argument);
}

TEST(Mat, InverseRoundTrips) {
    const Mat<float, 2, 2> m{{4.f, 7.f}, {2.f, 6.f}};
    const auto inv = m.inverse();
    ASSERT_TRUE(inv.has_value());
    const auto id = m * (*inv);
    EXPECT_NEAR((id[0, 0]), 1.f, 1e-5f);
    EXPECT_NEAR((id[1, 1]), 1.f, 1e-5f);
    EXPECT_NEAR((id[0, 1]), 0.f, 1e-5f);
    EXPECT_NEAR((id[1, 0]), 0.f, 1e-5f);
}

TEST(Mat, InverseOfSingularReturnsError) {
    const Mat<float, 2, 2> m{{1.f, 2.f}, {2.f, 4.f}}; // rows are linearly dependent
    const auto inv = m.inverse();
    ASSERT_FALSE(inv.has_value());
    EXPECT_EQ(inv.error(), MatrixError::SINGULAR);
}
