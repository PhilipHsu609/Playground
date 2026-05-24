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
    Mat<float, 2, 2> m{{1.f, 2.f}, {3.f, 4.f}};
    auto id = Mat<float, 2, 2>::identity();
    auto result = m * id;
    EXPECT_EQ((result[0, 0]), 1.f);
    EXPECT_EQ((result[0, 1]), 2.f);
    EXPECT_EQ((result[1, 0]), 3.f);
    EXPECT_EQ((result[1, 1]), 4.f);
}

TEST(Mat, MatrixMultiplyNonSquare) {
    // 2x3 * 3x2 = 2x2
    Mat<float, 2, 3> a{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}};
    Mat<float, 3, 2> b{{7.f, 8.f}, {9.f, 10.f}, {11.f, 12.f}};
    auto c = a * b;
    EXPECT_EQ((c[0, 0]), 58.f);  // 1*7 + 2*9 + 3*11
    EXPECT_EQ((c[0, 1]), 64.f);  // 1*8 + 2*10 + 3*12
    EXPECT_EQ((c[1, 0]), 139.f); // 4*7 + 5*9 + 6*11
    EXPECT_EQ((c[1, 1]), 154.f); // 4*8 + 5*10 + 6*12
}

TEST(Mat, MatrixVectorMultiplyIdentity) {
    auto m = Mat<float, 3, 3>::identity();
    Vec3f v(1.f, 2.f, 3.f);
    Vec3f result = m * v;
    EXPECT_EQ(result, v);
}

TEST(Mat, MatrixVectorMultiply) {
    // rotation by 90 deg around z-axis: x -> y, y -> -x
    Mat<float, 3, 3> m{{0.f, -1.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}};
    Vec3f v(1.f, 0.f, 0.f);
    Vec3f result = m * v;
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
