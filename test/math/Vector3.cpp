#include "gtest/gtest.h"

#include "math/Vector3.h"

namespace test
{

TEST(Vector3, Constructor)
{
    Vector3 vec(1.0, 2.0, 3.5);

    EXPECT_EQ(vec.x(), 1.0);
    EXPECT_EQ(vec.y(), 2.0);
    EXPECT_EQ(vec.z(), 3.5);
}

}
