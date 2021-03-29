#include "gtest/gtest.h"

#include "math/Vector3.h"

namespace test
{

TEST(MathTest, ConstructVector3)
{
    Vector3 vec(1.0, 2.0, 3.5);

    EXPECT_EQ(vec.x(), 1.0);
    EXPECT_EQ(vec.y(), 2.0);
    EXPECT_EQ(vec.z(), 3.5);
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
