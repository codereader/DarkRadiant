#include "gtest/gtest.h"

#include "math/Vector3.h"
#include "math/Vector4.h"

namespace test
{

TEST(VectorTest, ConstructVector3)
{
    Vector3 vec(1.0, 2.0, 3.5);

    EXPECT_EQ(vec.x(), 1.0);
    EXPECT_EQ(vec.y(), 2.0);
    EXPECT_EQ(vec.z(), 3.5);
}

TEST(VectorTest, DefaultConstructVector3)
{
    Vector3 vec;

    EXPECT_EQ(vec, Vector3(0, 0, 0));
}

TEST(VectorTest, ConstructVector3FromArray)
{
    double values[3] = { 1.0, 14.0, -96.5 };
    Vector3 vec(values);

    EXPECT_EQ(vec, Vector3(1.0, 14.0, -96.5));
}

TEST(VectorTest, ConstructVector4)
{
    Vector4 vec(25.0, -356, 128.25, 14);

    EXPECT_EQ(vec.x(), 25.0);
    EXPECT_EQ(vec.y(), -356);
    EXPECT_EQ(vec.z(), 128.25);
    EXPECT_EQ(vec.w(), 14);
}

TEST(VectorTest, ConstructVector4DefaultW)
{
    Vector4 vec(16, 2, 8);

    EXPECT_EQ(vec.w(), 1);
}

TEST(VectorTest, DefaultConstructVector4)
{
    Vector4 vec;

    EXPECT_EQ(vec, Vector4(0, 0, 0, 0));
}

TEST(VectorTest, SetVector3Components)
{
    Vector3 vec(-16, 256, 0.95);

    // Set values with the set() method
    vec.set(156, -83, -1.25);
    EXPECT_EQ(vec, Vector3(156, -83, -1.25));

    // Set values with the reference accessors
    vec.x() = -20;
    vec.y() = 1230;
    vec.z() = 9;
    EXPECT_EQ(vec, Vector3(-20, 1230, 9));
}

TEST(VectorTest, PromoteVector3To4)
{
    Vector3 v3(8, 12, -5);
    Vector4 v4(v3);

    EXPECT_EQ(v4, Vector4(8, 12, -5, 1));
}

TEST(VectorTest, PromoteVector3To4WithDifferentType)
{
    BasicVector3<float> v3(-9, 12, 47);
    BasicVector4<double> v4(v3);

    EXPECT_EQ(v4, Vector4(-9, 12, 47, 1));
}

TEST(VectorTest, DemoteVector4To3)
{
    Vector4 v4(1, -96, 0.125);
    Vector3 v3 = v4.getVector3();

    EXPECT_EQ(v3, Vector3(1, -96, 0.125));
}

TEST(VectorTest, Vector3EqualityComparison)
{
    Vector3 v1(1, 2, 8);
    Vector3 v1a(1, 2, 8);
    Vector3 v2(9, 14, -0.5);

    EXPECT_EQ(v1, v1);
    EXPECT_EQ(v1, v1a);
    EXPECT_NE(v1, v2);
}

TEST(VectorTest, Vector3EpsilonComparison)
{
    const Vector3 v(1, 8, 320);
    const Vector3 increment(1e-8, 1e-7, 1e-8);

    EXPECT_TRUE(math::isNear(v, v, 1e-8));
    EXPECT_TRUE(math::isNear(v, v + increment, 1e-6));
    EXPECT_TRUE(math::isNear(v, v - increment, 1e-6));
    EXPECT_FALSE(math::isNear(v, v + increment, 1e-9));
    EXPECT_FALSE(math::isNear(v, v - increment, 1e-9));
    EXPECT_TRUE(math::isNear(v, v + Vector3(1000, 10000, 20000), 1e6));
}

TEST(VectorTest, Vector4EqualityComparison)
{
    Vector4 v1(-5, 27, 9562);
    Vector4 v1a(-5, 27, 9562);
    Vector4 v2(350, 0.025, 16);

    EXPECT_EQ(v1, v1);
    EXPECT_EQ(v1, v1a);
    EXPECT_NE(v1, v2);
}

TEST(VectorTest, Vector4EpsilonComparison)
{
    const Vector4 v(256, -18, -0.5);
    const Vector4 increment(1e-8, 1e-8, 1e-8, 1e-8);

    EXPECT_NE(v, v + increment);
    EXPECT_TRUE(math::isNear(v, v, 1e-10));
    EXPECT_TRUE(math::isNear(v, v + increment, 1e-6));
    EXPECT_TRUE(math::isNear(v, v - increment, 1e-6));
    EXPECT_FALSE(math::isNear(v, v + increment, 1e-10));
    EXPECT_FALSE(math::isNear(v, v - increment, 1e-10));
}

TEST(VectorTest, NegateVector3)
{
    Vector3 vec(5, 10, 125);

    EXPECT_EQ(-vec, Vector3(-5, -10, -125));
    EXPECT_EQ(-(-vec), vec);
}

TEST(VectorTest, VectorLength)
{
    const Vector3 vA(3, 4, 5);
    EXPECT_EQ(vA.getLengthSquared(), 3*3 + 4*4 + 5*5);
    EXPECT_EQ(vA.getLength(), sqrt(3*3 + 4*4 + 5*5));

    const Vector3 vB(-2, 0.5, 16);
    EXPECT_EQ(vB.getLengthSquared(), 4 + 0.25 + 256);
    EXPECT_EQ(vB.getLength(), sqrt(4 + 0.25 + 256));
}

TEST(VectorTest, NormaliseVector3)
{
    Vector3 v(10, 5, 4);
    Vector3 vN = v.getNormalised();

    // getNormalised should return correct result and not change the original
    EXPECT_TRUE(math::isNear(vN, v * 1.0/v.getLength(), 10E-6));
    EXPECT_NE(vN, v);

    // Normalise in place gives same result
    v.normalise();
    EXPECT_EQ(v, vN);
}

TEST(VectorTest, AddVector3)
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

TEST(VectorTest, AddVector4)
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

TEST(VectorTest, SubtractVector3)
{
    Vector3 v1(-9, 14.5, 650);
    const Vector3 v1Orig = v1;
    const Vector3 v2(4, 18, -7.5);

    // Subtract and return
    const Vector3 diff = v1 - v2;
    EXPECT_EQ(diff, Vector3(-13, -3.5, 657.5));

    // Subtract in place
    EXPECT_EQ(v1, v1Orig);
    v1 -= v2;
    EXPECT_EQ(v1, diff);
}

TEST(VectorTest, SubtractVector4)
{
    Vector4 v1(0, 96, 457, -3.5);
    Vector4 v2(0.125, 4, 7.5, 90);

    // Subtract and return
    EXPECT_EQ(v1 - v2, Vector4(-0.125, 92, 449.5, -93.5));
    EXPECT_EQ(v2 - v2, Vector4());

    // Subtract in place
    auto v1Copy = v1;
    v1 -= v2;
    EXPECT_NE(v1, v1Copy);
    EXPECT_EQ(v1, Vector4(-0.125, 92, 449.5, -93.5));
}

TEST(VectorTest, ScalarMultiplyVector3)
{
    Vector3 vec(2, 4, -5);
    const Vector3 vecOrig = vec;

    // Multiply and return
    EXPECT_EQ(vec * 2, Vector3(4, 8, -10));
    EXPECT_EQ(2 * vec, Vector3(4, 8, -10));

    EXPECT_EQ(vec * 0.5, Vector3(1, 2, -2.5));
    EXPECT_EQ(0.5 * vec, Vector3(1, 2, -2.5));

    // Multiply in place
    EXPECT_EQ(vec, vecOrig);
    vec *= 3;
    EXPECT_EQ(vec, Vector3(6, 12, -15));
}

TEST(VectorTest, ScalarDivideVector3)
{
    Vector3 vec(-6, 12, 36);
    const Vector3 orig = vec;

    // Divide and return
    EXPECT_EQ(vec / 2, Vector3(-3, 6, 18));
    EXPECT_EQ(vec / 3.0, Vector3(-2, 4, 12));
    EXPECT_EQ(vec / -1, Vector3(6, -12, -36));

    // Divide in place
    EXPECT_EQ(vec, orig);
    vec /= 1;
    EXPECT_EQ(vec, orig);
    vec /= 2;
    EXPECT_EQ(vec, Vector3(-3, 6, 18));
    vec /= 3;
    EXPECT_EQ(vec, Vector3(-1, 2, 6));
}

TEST(VectorTest, ScalarDivideVector4)
{
    Vector4 vec(32, 168, -25, 0.8);

    // Divide and return
    EXPECT_EQ(vec / 2, Vector4(16, 84, -12.5, 0.4));
    EXPECT_EQ(vec / 0.5, Vector4(64, 336, -50, 1.6));
    EXPECT_EQ(vec / -1, Vector4(-32, -168, 25, -0.8));

    // Divide in place
    const Vector4 orig = vec;
    EXPECT_EQ(vec, orig);
    vec /= 2;
    EXPECT_EQ(vec, Vector4(16, 84, -12.5, 0.4));
}

TEST(VectorTest, ScalarMultiplyVector4)
{
    Vector4 vec(8, -14, 26, 1.8);

    EXPECT_EQ(vec * 2, Vector4(16, -28, 52, 3.6));
    EXPECT_EQ(2 * vec, Vector4(16, -28, 52, 3.6));

    EXPECT_EQ(vec * 0.5, Vector4(4, -7, 13, 0.9));
    EXPECT_EQ(0.5 * vec, Vector4(4, -7, 13, 0.9));

    // Multiply in place
    Vector4 v2 = vec * 3.5;
    EXPECT_NE(vec, v2);
    vec *= 3.5;
    EXPECT_EQ(vec, v2);
}

TEST(VectorTest, ComponentwiseMultiplyVector3)
{
    Vector3 v(0.8, 24, -300);
    const Vector3 vOrig = v;
    const Vector3 scale(2, 0.5, 3);

    // Multiply and return
    const Vector3 prod = v * scale;
    EXPECT_EQ(prod, Vector3(1.6, 12, -900));
    EXPECT_EQ(scale * v, prod);

    // Multiply in place
    EXPECT_EQ(v, vOrig);
    v *= scale;
    EXPECT_EQ(v, prod);
}

TEST(VectorTest, ComponentwiseDivideVector3)
{
    Vector3 vec(320, -240, 128);
    const Vector3 vOrig = vec;
    const Vector3 scale(2, 3, -4);

    // Divide and return
    const Vector3 result = vec / scale;
    EXPECT_EQ(result, Vector3(160, -80, -32));
    EXPECT_EQ(result / (1.0 / scale), vOrig);

    // Divide in place
    EXPECT_EQ(vec, vOrig);
    vec /= scale;
    EXPECT_EQ(vec, result);
}

TEST(VectorTest, Vector3DotProduct)
{
    const Vector3 v1(2, 4, 30);
    const Vector3 v2(8, 0.25, -5);
    const Vector3 v3(0.5, 2, 0.25);

    EXPECT_EQ(v1.dot(v2), 16 + 1 - 150);
    EXPECT_EQ(v1.dot(v3), 1 + 8 + 7.5);
    EXPECT_EQ(v2.dot(v3), 4 + 0.5 - 1.25);

    EXPECT_EQ(v1.dot(v1), v1.getLengthSquared());
    EXPECT_EQ(v2.dot(v2), v2.getLengthSquared());
    EXPECT_EQ(v3.dot(v3), v3.getLengthSquared());
}

TEST(VectorTest, Vector4DotProduct)
{
    const Vector4 v1(-8, 0.5, 128, 26);
    const Vector4 v2(0.5, 256, -18, 4);
    const Vector4 v3(25, 0.1, 0.75, 800);

    EXPECT_EQ(v1.dot(v2), -2076);
    EXPECT_EQ(v2.dot(v1), v1.dot(v2));
    EXPECT_EQ(v2.dot(v3), 3224.6);
    EXPECT_EQ(v3.dot(v2), v2.dot(v3));
}

TEST(VectorTest, Vector3CrossProduct)
{
    const Vector3 i(1, 0, 0);
    const Vector3 j(0, 1, 0);
    const Vector3 k(0, 0, 1);

    // Validate the cross product unit vector identities
    EXPECT_EQ(i.cross(j), k);
    EXPECT_EQ(j.cross(i), -k);
    EXPECT_EQ(i.cross(k), -j);
    EXPECT_EQ(k.cross(i), j);
    EXPECT_EQ(j.cross(k), i);
    EXPECT_EQ(k.cross(j), -i);

    const Vector3 ZERO(0, 0, 0);
    EXPECT_EQ(i.cross(i), ZERO);
    EXPECT_EQ(j.cross(j), ZERO);
    EXPECT_EQ(k.cross(k), ZERO);

    // Check with non-unit vectors
    EXPECT_EQ((2 * i).cross(3 * j), 6 * k);
    EXPECT_EQ((0.5 * k).cross(8 * j), -4 * i);

    // Arbitrary vectors
    EXPECT_EQ(Vector3(2, 4, 5).cross(Vector3(3, 5, 9)), Vector3(11, -3, -2));
    EXPECT_EQ(Vector3(-0.25, 4.8, 512).cross(Vector3(56, -5.625, 96)),
              Vector3(3340.8, 28696.0, -267.39375));
}

TEST(VectorTest, AngleBetweenVector3s)
{
    const Vector3 i(1, 0, 0);
    const Vector3 j(0, 1, 0);
    const Vector3 k(0, 0, 1);

    // Obvious unit vector angles
    EXPECT_DOUBLE_EQ(i.angle(i), 0);
    EXPECT_DOUBLE_EQ(i.angle(j), math::PI / 2.0);
    EXPECT_DOUBLE_EQ(i.angle(k), math::PI / 2.0);
    EXPECT_DOUBLE_EQ(j.angle(k), math::PI / 2.0);
    EXPECT_DOUBLE_EQ(j.angle(-j), math::PI);

    // Arbitrary vectors
    EXPECT_DOUBLE_EQ(Vector3(2, 4, 5).angle(Vector3(4, 9, -7)),
                     1.4595319160396403);
    EXPECT_DOUBLE_EQ(Vector3(0.1, -0.5, 19).angle(Vector3(2.4, 156.1, -27.8)),
                     1.7732415935926593);
}

TEST(VectorTest, VectorIsParallel)
{
    const Vector3 i(1, 0, 0);
    const Vector3 j(0, 1, 0);
    const Vector3 k(0, 0, 1);

    // Vectors are parallel to themselves and not parallel to other unit vectors
    EXPECT_TRUE(math::isParallel(i, i));
    EXPECT_TRUE(math::isParallel(j, j));
    EXPECT_FALSE(math::isParallel(i, j));
    EXPECT_FALSE(math::isParallel(k, i));

    // Vectors are parallel even in opposite directions
    EXPECT_TRUE(math::isParallel(i, -i));
    EXPECT_TRUE(math::isParallel(j, -j));
    EXPECT_TRUE(math::isParallel(k, -k));

    // Scaling does not change parallelism
    EXPECT_TRUE(math::isParallel(i * 2, i));
    EXPECT_TRUE(math::isParallel(j * 356, j * 0.05));
}

TEST(VectorTest, SnapVector3)
{
    Vector3 v1(5, -15.25, -28);
    const Vector3 v1Orig = v1;

    // Non-mutating version
    EXPECT_EQ(v1.getSnapped(0.25), v1);
    EXPECT_EQ(v1.getSnapped(1), Vector3(5, -15, -28));
    EXPECT_EQ(v1.getSnapped(2), Vector3(4, -16, -28));
    EXPECT_EQ(v1.getSnapped(5), Vector3(5, -15, -30));

    // Mutating version
    EXPECT_EQ(v1, v1Orig);
    v1.snap(3);
    EXPECT_EQ(v1, Vector3(6, -15, -27));
}

TEST(VectorTest, Vector3MidPoint)
{
    Vector3 v1(18, 26, -350);
    Vector3 v2(-4.5, 96, 10);

    EXPECT_EQ(math::midPoint(v1, v2), Vector3(6.75, 61, -170));
    EXPECT_EQ(math::midPoint(v2, v1), Vector3(6.75, 61, -170));
    EXPECT_EQ(math::midPoint(v1, Vector3()), v1 / 2);
}

TEST(VectorTest, Vector3AsCArray)
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

TEST(VectorTest, Vector3fAsCArray)
{
    Vector3f vec(-0.5, 485, 0);

    EXPECT_EQ(&vec.y(), &vec.x() + 1);
    EXPECT_EQ(&vec.z(), &vec.y() + 1);

    float* array = vec;
    EXPECT_EQ(array[0], -0.5);
    EXPECT_EQ(array[1], 485);
    EXPECT_EQ(array[2], 0);
}

TEST(VectorTest, Vector4AsCArray)
{
    Vector4 vec(-15, 15, 36.9, -0.5);

    EXPECT_EQ(&vec.y(), &vec.x() + 1);
    EXPECT_EQ(&vec.z(), &vec.y() + 1);
    EXPECT_EQ(&vec.w(), &vec.z() + 1);

    double* array = vec;
    EXPECT_EQ(array[0], -15);
    EXPECT_EQ(array[1], 15);
    EXPECT_EQ(array[2], 36.9);
    EXPECT_EQ(array[3], -0.5);
}

TEST(VectorTest, ProjectVector4To3)
{
    Vector4 v1(128, 56, 5, 1);
    Vector4 v2(1500, -12, 18, 2);

    EXPECT_EQ(v1.getProjected(), Vector3(128, 56, 5));
    EXPECT_EQ(v2.getProjected(), Vector3(750, -6, 9));
    v2.w() = 3;
    EXPECT_EQ(v2.getProjected(), Vector3(500, -4, 6));
}

TEST(VectorTest, TruncateVector4To3)
{
    // Const truncation
    const Vector4 v1(56, 58, 40, 2);
    Vector3 v1Trunc = v1.getVector3();
    EXPECT_EQ(v1Trunc, Vector3(56, 58, 40));
    EXPECT_NE(v1Trunc, v1.getProjected());
}

}
