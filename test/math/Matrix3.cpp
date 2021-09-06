#include "gtest/gtest.h"

#include "math/Matrix3.h"

namespace test
{

TEST(Matrix3Test, CreateIdentityMatrix)
{
    const auto identity = Matrix3::getIdentity();
    EXPECT_EQ(identity, Matrix3::byRows(1, 0, 0,
        0, 1, 0,
        0, 0, 1));
}

}
