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

TEST(Matrix3Test, AssignMatrixComponents)
{
    Matrix3 identity;

    identity.xx() = 1;
    identity.xy() = 0;
    identity.xz() = 0;

    identity.yx() = 0;
    identity.yy() = 1;
    identity.yz() = 0;

    identity.zx() = 0;
    identity.zy() = 0;
    identity.zz() = 1;

    EXPECT_EQ(identity, Matrix3::getIdentity());
}

TEST(Matrix3Test, ConstructMatrixByRows)
{
    auto m = Matrix3::byRows(1, 2.5, 3, 
        0.34, 51, -6, 
        7, 9, 17);

    // Check individual values
    EXPECT_EQ(m.xx(), 1);
    EXPECT_EQ(m.xy(), 0.34);
    EXPECT_EQ(m.xz(), 7);

    EXPECT_EQ(m.yx(), 2.5);
    EXPECT_EQ(m.yy(), 51);
    EXPECT_EQ(m.yz(), 9);

    EXPECT_EQ(m.zx(), 3);
    EXPECT_EQ(m.zy(), -6);
    EXPECT_EQ(m.zz(), 17);
}

TEST(Matrix3Test, AccessMatrixColumnVectors)
{
    Matrix3 m = Matrix3::byRows(1, 4, 8, 
        2, 9, 7,
        11, -2, 10);

    // Read column values
    EXPECT_EQ(m.xCol(), Vector3(1, 2, 11));
    EXPECT_EQ(m.yCol(), Vector3(4, 9, -2));
    EXPECT_EQ(m.zCol(), Vector3(8, 7, 10));

    // Write column values
    m.xCol() = Vector3(0.1, 0.2, 0.3);
    m.yCol() = Vector3(0.5, 0.6, 0.7);
    m.zCol() = Vector3(0.9, 1.0, 1.1);
    EXPECT_EQ(m, Matrix3::byColumns(0.1, 0.2, 0.3,
        0.5, 0.6, 0.7,
        0.9, 1.0, 1.1));
}

TEST(Matrix3Test, MatrixEquality)
{
    Matrix3 m1 = Matrix3::byRows(1, 2, 3.5,
        5, -6, 17,
        9.01, 10, 11);
    Matrix3 m2 = m1;

    EXPECT_TRUE(m1 == m2);
    EXPECT_EQ(m1, m2);
    EXPECT_NE(m1, Matrix3::getIdentity());
    EXPECT_NE(m1, Matrix3::getIdentity());
    EXPECT_TRUE(m1 != Matrix3::getIdentity());
    EXPECT_TRUE(m2 != Matrix3::getIdentity());
}

}
