#include "gtest/gtest.h"

#include "math/Vector3.h"
#include "math/Vector4.h"

namespace test
{

TEST(MathTest, ConstructVector3)
{
    Vector3 vec(1.0, 2.0, 3.5);

    EXPECT_EQ(vec.x(), 1.0);
    EXPECT_EQ(vec.y(), 2.0);
    EXPECT_EQ(vec.z(), 3.5);
}

TEST(MathTest, DefaultConstructVector3)
{
    Vector3 vec;

    EXPECT_EQ(vec, Vector3(0, 0, 0));
}

TEST(MathTest, ConstructVector3FromArray)
{
    double values[3] = { 1.0, 14.0, -96.5 };
    Vector3 vec(values);

    EXPECT_EQ(vec, Vector3(1.0, 14.0, -96.5));
}

TEST(MathTest, PromoteVector3To4)
{
    Vector3 v3(8, 12, -5);
    Vector4 v4(v3);

    EXPECT_EQ(v4, Vector4(8, 12, -5, 1));
}

TEST(MathTest, DemoteVector4To3)
{
    Vector4 v4(1, -96, 0.125);
    Vector3 v3 = v4.getVector3();

    EXPECT_EQ(v3, Vector3(1, -96, 0.125));
}

TEST(MathTest, Vector3EqualityComparison)
{
    Vector3 v1(1, 2, 8);
    Vector3 v1a(1, 2, 8);
    Vector3 v2(9, 14, -0.5);

    EXPECT_EQ(v1, v1);
    EXPECT_EQ(v1, v1a);
    EXPECT_NE(v1, v2);
}

TEST(MathTest, NegateVector3)
{
    Vector3 vec(5, 10, 125);

    EXPECT_EQ(-vec, Vector3(-5, -10, -125));
}

TEST(MathTest, VectorLength)
{
    Vector3 v3(3, 4, 5);
    EXPECT_EQ(v3.getLengthSquared(), 3*3 + 4*4 + 5*5);
    EXPECT_NEAR(v3.getLength(), sqrt(3*3 + 4*4 + 5*5), 1E-6);
}

TEST(MathTest, NormaliseVector3)
{
    Vector3 v(10, 5, 4);
    Vector3 vN = v.getNormalised();

    // getNormalised should return correct result and not change the original
    EXPECT_EQ(vN, v * 1.0/v.getLength());
    EXPECT_NE(vN, v);

    // Normalise in place gives same result
    v.normalise();
    EXPECT_EQ(v, vN);
}

TEST(MathTest, AddVector3)
{
    Vector3 v1(2, -5, 17);
    const Vector3 v1Orig = v1;
    const Vector3 v2(11, 12, -0.5);

    // Add and return
    const Vector3 sum = v1 + v2;
    EXPECT_EQ(sum, Vector3(13, 7, 16.5));
    EXPECT_EQ(v1, v1Orig);

    // Add in place
    v1 += v2;
    EXPECT_EQ(v1, sum);
}

TEST(MathTest, AddVector4)
{
    Vector4 v1(10, 0.25, -60, 11);
    const Vector4 v1Orig = v1;
    const Vector4 v2(0, -18, 276, 0.75);

    // Add and return
    const Vector4 sum = v1 + v2;
    EXPECT_EQ(sum, Vector4(10, -17.75, 216, 11.75));
    EXPECT_EQ(v1, v1Orig);

    // Add in place
    v1 += v2;
    EXPECT_EQ(v1, sum);
}

TEST(MathTest, SubtractVector3)
{
    Vector3 v1(-9, 14.5, 650);
    Vector3 v2(4, 18, -7.5);

    EXPECT_EQ(v1 - v2, Vector3(-13, -3.5, 657.5));
}

TEST(MathTest, SubtractVector4)
{
    Vector4 v1(0, 96, 457, -3.5);
    Vector4 v2(0.125, 4, 7.5, 90);

    EXPECT_EQ(v1 - v2, Vector4(-0.125, 92, 449.5, -93.5));
}

TEST(MathTest, ScalarMultiplyVector3)
{
    Vector3 vec(2, 4, -5);

    EXPECT_EQ(vec * 2, Vector3(4, 8, -10));
    EXPECT_EQ(2 * vec, Vector3(4, 8, -10));

    EXPECT_EQ(vec * 0.5, Vector3(1, 2, -2.5));
    EXPECT_EQ(0.5 * vec, Vector3(1, 2, -2.5));
}

TEST(MathTest, ScalarMultiplyVector4)
{
    Vector4 vec(8, -14, 26, 1.8);

    EXPECT_EQ(vec * 2, Vector4(16, -28, 52, 3.6));
    EXPECT_EQ(2 * vec, Vector4(16, -28, 52, 3.6));

    EXPECT_EQ(vec * 0.5, Vector4(4, -7, 13, 0.9));
    EXPECT_EQ(0.5 * vec, Vector4(4, -7, 13, 0.9));
}

TEST(MathTest, Vector3AsCArray)
{
    Vector3 vec(256, -10, 10000);

    // Ensure that X, Y and Z are at consecutive memory locations and can be
    // treated as a 3-element array.
    EXPECT_EQ(&vec.y(), &vec.x() + 1);
    EXPECT_EQ(&vec.z(), &vec.y() + 1);

    double* array = vec;
    EXPECT_EQ(array[0], 256);
    EXPECT_EQ(array[1], -10);
    EXPECT_EQ(array[2], 10000);
}

TEST(MathTest, Vector3fAsCArray)
{
    Vector3f vec(-0.5, 485, 0);

    EXPECT_EQ(&vec.y(), &vec.x() + 1);
    EXPECT_EQ(&vec.z(), &vec.y() + 1);

    float* array = vec;
    EXPECT_EQ(array[0], -0.5);
    EXPECT_EQ(array[1], 485);
    EXPECT_EQ(array[2], 0);
}

}
