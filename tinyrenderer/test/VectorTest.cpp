#include "tinyrenderer/Vector.hpp"

#include <gtest/gtest.h>

// --- Construction and access ---

TEST(Vec, DefaultConstructionZeroInitializes) {
    Vec3f v;
    EXPECT_EQ(v.x(), 0.f);
    EXPECT_EQ(v.y(), 0.f);
    EXPECT_EQ(v.z(), 0.f);
}

TEST(Vec, ValueConstruction) {
    Vec3f v(1.f, 2.f, 3.f);
    EXPECT_EQ(v.x(), 1.f);
    EXPECT_EQ(v.y(), 2.f);
    EXPECT_EQ(v.z(), 3.f);
}

TEST(Vec, NamedAccessorsAreReferences) {
    Vec3f v(1.f, 2.f, 3.f);
    v.x() = 10.f;
    v.y() = 20.f;
    v.z() = 30.f;
    EXPECT_EQ(v[0], 10.f);
    EXPECT_EQ(v[1], 20.f);
    EXPECT_EQ(v[2], 30.f);
}

TEST(Vec, Vec4NamedAccessors) {
    Vec4f v(1.f, 2.f, 3.f, 4.f);
    EXPECT_EQ(v.x(), 1.f);
    EXPECT_EQ(v.y(), 2.f);
    EXPECT_EQ(v.z(), 3.f);
    EXPECT_EQ(v.w(), 4.f);
}

TEST(Vec, Clear) {
    Vec3f v(1.f, 2.f, 3.f);
    v.clear();
    EXPECT_EQ(v, Vec3f(0.f, 0.f, 0.f));
}

// --- Type and size conversion ---

TEST(Vec, FloatToIntTruncates) {
    Vec3f f(1.9f, -2.7f, 3.1f);
    Vec3i i(f);
    EXPECT_EQ(i.x(), 1);
    EXPECT_EQ(i.y(), -2);
    EXPECT_EQ(i.z(), 3);
}

TEST(Vec, ShrinkDropsTrailingComponents) {
    Vec3f v3(1.f, 2.f, 3.f);
    Vec2f v2(v3);
    EXPECT_EQ(v2.x(), 1.f);
    EXPECT_EQ(v2.y(), 2.f);
}

TEST(Vec, GrowZeroPadsNewComponents) {
    Vec2f v2(1.f, 2.f);
    Vec3f v3(v2);
    EXPECT_EQ(v3.x(), 1.f);
    EXPECT_EQ(v3.y(), 2.f);
    EXPECT_EQ(v3.z(), 0.f);
}

// --- Arithmetic ---

TEST(Vec, Addition) {
    EXPECT_EQ(Vec3f(1.f, 2.f, 3.f) + Vec3f(4.f, 5.f, 6.f), Vec3f(5.f, 7.f, 9.f));
}

TEST(Vec, Subtraction) {
    EXPECT_EQ(Vec3f(4.f, 5.f, 6.f) - Vec3f(1.f, 2.f, 3.f), Vec3f(3.f, 3.f, 3.f));
}

TEST(Vec, ScalarMultiply) { EXPECT_EQ(Vec3f(1.f, 2.f, 3.f) * 2.f, Vec3f(2.f, 4.f, 6.f)); }

TEST(Vec, ScalarMultiplyCommutative) {
    EXPECT_EQ(2.f * Vec3f(1.f, 2.f, 3.f), Vec3f(1.f, 2.f, 3.f) * 2.f);
}

TEST(Vec, ScalarDivide) { EXPECT_EQ(Vec3f(2.f, 4.f, 6.f) / 2.f, Vec3f(1.f, 2.f, 3.f)); }

TEST(Vec, CompoundAddition) {
    Vec3f v(1.f, 2.f, 3.f);
    v += Vec3f(4.f, 5.f, 6.f);
    EXPECT_EQ(v, Vec3f(5.f, 7.f, 9.f));
}

TEST(Vec, CompoundSubtraction) {
    Vec3f v(4.f, 5.f, 6.f);
    v -= Vec3f(1.f, 2.f, 3.f);
    EXPECT_EQ(v, Vec3f(3.f, 3.f, 3.f));
}

TEST(Vec, CompoundScalarMultiply) {
    Vec3f v(1.f, 2.f, 3.f);
    v *= 3.f;
    EXPECT_EQ(v, Vec3f(3.f, 6.f, 9.f));
}

TEST(Vec, CompoundScalarDivide) {
    Vec3f v(3.f, 6.f, 9.f);
    v /= 3.f;
    EXPECT_EQ(v, Vec3f(1.f, 2.f, 3.f));
}

// --- Dot product ---

TEST(Vec, DotProduct) {
    EXPECT_EQ(dot(Vec3f(1.f, 2.f, 3.f), Vec3f(4.f, 5.f, 6.f)), 32.f);
}

TEST(Vec, DotProductCommutative) {
    Vec3f a(1.f, 2.f, 3.f);
    Vec3f b(4.f, 5.f, 6.f);
    EXPECT_EQ(dot(a, b), dot(b, a));
}

TEST(Vec, DotProductPerpendicularIsZero) {
    EXPECT_EQ(dot(Vec3f(1.f, 0.f, 0.f), Vec3f(0.f, 1.f, 0.f)), 0.f);
    EXPECT_EQ(dot(Vec3f(1.f, 0.f, 0.f), Vec3f(0.f, 0.f, 1.f)), 0.f);
}

TEST(Vec, DotProductParallelIsProductOfLengths) {
    Vec3f v(3.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(dot(v, v), 9.f);
}

// --- Cross product ---

TEST(Vec, CrossProductBasisVectors) {
    Vec3f x(1.f, 0.f, 0.f);
    Vec3f y(0.f, 1.f, 0.f);
    Vec3f z(0.f, 0.f, 1.f);
    EXPECT_EQ(cross(x, y), z);
    EXPECT_EQ(cross(y, z), x);
    EXPECT_EQ(cross(z, x), y);
}

TEST(Vec, CrossProductAntiCommutative) {
    Vec3f a(1.f, 2.f, 3.f);
    Vec3f b(4.f, 5.f, 6.f);
    Vec3f ab = cross(a, b);
    Vec3f ba = cross(b, a);
    EXPECT_EQ(ab, Vec3f(0.f, 0.f, 0.f) - ba);
}

TEST(Vec, CrossProductPerpendicularToInputs) {
    Vec3f a(1.f, 2.f, 3.f);
    Vec3f b(4.f, 5.f, 6.f);
    Vec3f c = cross(a, b);
    EXPECT_FLOAT_EQ(dot(c, a), 0.f);
    EXPECT_FLOAT_EQ(dot(c, b), 0.f);
}

TEST(Vec, CrossProductParallelIsZero) {
    Vec3f v(1.f, 2.f, 3.f);
    EXPECT_EQ(cross(v, v), Vec3f(0.f, 0.f, 0.f));
    EXPECT_EQ(cross(v, v * 2.f), Vec3f(0.f, 0.f, 0.f));
}

// --- Norm and normalize ---

TEST(Vec, NormOfUnitVector) {
    EXPECT_DOUBLE_EQ(Vec3f(1.f, 0.f, 0.f).norm(), 1.0);
    EXPECT_DOUBLE_EQ(Vec3f(0.f, 1.f, 0.f).norm(), 1.0);
    EXPECT_DOUBLE_EQ(Vec3f(0.f, 0.f, 1.f).norm(), 1.0);
}

TEST(Vec, NormOfZeroVector) { EXPECT_DOUBLE_EQ(Vec3f(0.f, 0.f, 0.f).norm(), 0.0); }

TEST(Vec, LengthAliasesNorm) {
    Vec3f v(3.f, 4.f, 0.f);
    EXPECT_DOUBLE_EQ(v.length(), v.norm());
}

TEST(Vec, NormPythagorean) { EXPECT_DOUBLE_EQ(Vec3f(3.f, 4.f, 0.f).norm(), 5.0); }

TEST(Vec, NormalizeProducesUnitLength) {
    Vec3f v(3.f, 4.f, 0.f);
    EXPECT_NEAR(v.normalize().norm(), 1.0, 1e-6);
}

TEST(Vec, NormalizePreservesDirection) {
    Vec3f v(0.f, 0.f, 5.f);
    Vec3f n = v.normalize();
    EXPECT_NEAR(n.x(), 0.f, 1e-6);
    EXPECT_NEAR(n.y(), 0.f, 1e-6);
    EXPECT_NEAR(n.z(), 1.f, 1e-6);
}

// --- Equality ---

TEST(Vec, EqualVectors) { EXPECT_EQ(Vec3f(1.f, 2.f, 3.f), Vec3f(1.f, 2.f, 3.f)); }

TEST(Vec, UnequalVectors) { EXPECT_NE(Vec3f(1.f, 2.f, 3.f), Vec3f(1.f, 2.f, 4.f)); }

// --- Works with Vec2 ---

TEST(Vec, Vec2Arithmetic) {
    Vec2i a(3, 4);
    Vec2i b(1, 2);
    EXPECT_EQ(a + b, Vec2i(4, 6));
    EXPECT_EQ(a - b, Vec2i(2, 2));
    EXPECT_EQ(dot(a, b), 11);
}
