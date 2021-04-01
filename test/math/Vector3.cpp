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

TEST(MathTest, VectorLength)
{
    Vector3 v3(3, 4, 5);
    EXPECT_EQ(v3.getLengthSquared(), 3*3 + 4*4 + 5*5);
    EXPECT_NEAR(v3.getLength(), sqrt(3*3 + 4*4 + 5*5), 1E-6);
}

TEST(MathTest, Vector3IsPacked)
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

TEST(MathTest, Vector3fIsPacked)
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
