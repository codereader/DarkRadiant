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

TEST(Matrix3Test, MatrixMultiplication)
{
    auto a = Matrix3::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29);
    auto b = Matrix3::byColumns(61, 67, 71, 73, 79, 83, 89, 97, 101);

    // Check multiplied result
    auto c = a.getMultipliedBy(b);
    EXPECT_EQ(c, Matrix3::byColumns(2269, 2809, 3625,
        2665, 3301, 4261,
        3253, 4029, 5201));

    // Multiplication has not changed original
    EXPECT_NE(a, c);

    // Check operator multiplication as well
    EXPECT_EQ(a * b, c);

    // Test Pre-Multiplication
    EXPECT_EQ(b.getMultipliedBy(a), a.getPremultipliedBy(b)) << "Matrix pre-multiplication mismatch";
}

TEST(Matrix3Test, MatrixFullInverse)
{
    auto a = Matrix3::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29);

    auto inv = a.getFullInverse();

    EXPECT_DOUBLE_EQ(inv.xx(), -7.0 / 10) << "Matrix inversion failed on xx";
    EXPECT_DOUBLE_EQ(inv.xy(), 4.0 / 5) << "Matrix inversion failed on xy";
    EXPECT_DOUBLE_EQ(inv.xz(), -3.0 / 10) << "Matrix inversion failed on xz";

    EXPECT_DOUBLE_EQ(inv.yx(), 1.0 / 5) << "Matrix inversion failed on yx";
    EXPECT_DOUBLE_EQ(inv.yy(), -23.0 / 10) << "Matrix inversion failed on yy";
    EXPECT_DOUBLE_EQ(inv.yz(), 13.0 / 10) << "Matrix inversion failed on yz";

    EXPECT_DOUBLE_EQ(inv.zx(), 3.0 / 10) << "Matrix inversion failed on zx";
    EXPECT_DOUBLE_EQ(inv.zy(), 13.0 / 10) << "Matrix inversion failed on zy";
    EXPECT_DOUBLE_EQ(inv.zz(), -4.0 / 5) << "Matrix inversion failed on zz";
}

TEST(Matrix3Test, MatrixTransformation)
{
    auto a = Matrix3::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29);

    {
        Vector2 v(61, 67);

        Vector2 transformed = a.transformPoint(v);

        EXPECT_EQ(transformed.x(), 939) << "Vector2 transformation failed";
        EXPECT_EQ(transformed.y(), 1199) << "Vector2 transformation failed";

        EXPECT_EQ(a * v, a.transformPoint(v));
    }

    {
        Vector3 vector(83, 89, 97);
        Vector3 transformed = a.transform(vector);

        EXPECT_EQ(transformed.x(), 3071) << "Vector3 transformation failed";
        EXPECT_EQ(transformed.y(), 3803) << "Vector3 transformation failed";
        EXPECT_EQ(transformed.z(), 4907) << "Vector3 transformation failed";

        EXPECT_EQ(a * vector, a.transform(vector));
    }
}

TEST(Matrix3Test, ConstructTranslationMatrix)
{
    const Vector2 translation(1.5, -2939);
    auto tm = Matrix3::getTranslation(translation);

    EXPECT_EQ(tm, Matrix3::byRows(1, 0, translation.x(), 0, 1, translation.y(), 0, 0, 1));
}

TEST(Matrix3Test, ConstructRotationMatrix)
{
    const double angle = degrees_to_radians(15);
    auto tm = Matrix3::getRotation(angle);

    EXPECT_EQ(tm, Matrix3::byRows(cos(angle), -sin(angle), 0,
        sin(angle), cos(angle), 0,
        0, 0, 1));
}

TEST(Matrix3Test, ConstructScaleMatrix)
{
    const Vector2 scale(1.5, -3.2);
    auto tm = Matrix3::getScale(scale);

    EXPECT_EQ(tm, Matrix3::byRows(scale.x(), 0, 0,
        0, scale.y(), 0,
        0, 0, 1));
}

}
