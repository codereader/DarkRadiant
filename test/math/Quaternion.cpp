#include "gtest/gtest.h"

#include "math/Quaternion.h"
#include "math/Matrix4.h"

namespace test
{

// Fuzzy equality assertion for Quaternions
void expectNear(const Quaternion& q1, const Quaternion& q2)
{
    EXPECT_DOUBLE_EQ(q1.x(), q2.x());
    EXPECT_DOUBLE_EQ(q1.y(), q2.y());
    EXPECT_DOUBLE_EQ(q1.z(), q2.z());
    EXPECT_DOUBLE_EQ(q1.w(), q2.w());
}

TEST(MathTest, QuaternionForXRotation)
{
    math::Degrees angle(45);
    Quaternion xRot = Quaternion::createForX(angle.asRadians());

    expectNear(xRot, Quaternion(sin(angle.asRadians() / 2), 0, 0,
                                cos(angle.asRadians() / 2)));
}

TEST(MathTest, QuaternionForYRotation)
{
    math::Degrees angle(60);
    Quaternion yRot = Quaternion::createForY(angle.asRadians());

    expectNear(yRot, Quaternion(0, sin(angle.asRadians() / 2), 0,
                                cos(angle.asRadians() / 2)));
}

TEST(MathTest, QuaternionForZRotation)
{
    math::Degrees angle(75);
    Quaternion zRot = Quaternion::createForZ(angle.asRadians());

    expectNear(zRot, Quaternion(0, 0, sin(angle.asRadians() / 2),
                                cos(angle.asRadians() / 2)));
}

TEST(MathTest, QuaternionMultiplication)
{
    Quaternion q1(3, 5, 7, 11);
    Quaternion q2(13, 17, 19, 23);

    Quaternion product = q1.getMultipliedBy(q2);

    EXPECT_EQ(product.x(), 188) << "Quaternion multiplication failed on x";
    EXPECT_EQ(product.y(), 336) << "Quaternion multiplication failed on y";
    EXPECT_EQ(product.z(), 356) << "Quaternion multiplication failed on z";
    EXPECT_EQ(product.w(), -4) << "Quaternion multiplication failed on w";
}

TEST(MathTest, QuaternionInPlaceMultiplication)
{
    Quaternion q1(3, 5, 7, 11);
    Quaternion q2(13, 17, 19, 23);
    Quaternion q1multiplied = q1;
    q1multiplied.multiplyBy(q2);

    EXPECT_EQ(q1multiplied.x(), 188) << "Quaternion in-place multiplication failed on x";
    EXPECT_EQ(q1multiplied.y(), 336) << "Quaternion in-place multiplication failed on y";
    EXPECT_EQ(q1multiplied.z(), 356) << "Quaternion in-place multiplication failed on z";
    EXPECT_EQ(q1multiplied.w(), -4) << "Quaternion in-place multiplication failed on w";
}

TEST(MathTest, QuaternionInverse)
{
    Quaternion q1(3, 5, 7, 11);
    Quaternion q1inverted = q1.getInverse();

    EXPECT_EQ(q1inverted.x(), -3) << "Quaternion inversion failed on x";
    EXPECT_EQ(q1inverted.y(), -5) << "Quaternion inversion failed on y";
    EXPECT_EQ(q1inverted.z(), -7) << "Quaternion inversion failed on z";
    EXPECT_EQ(q1inverted.w(), 11) << "Quaternion inversion failed on w";
}

TEST(MathTest, QuaternionNormalised)
{
    Quaternion q1(3, 5, 7, 11);
    Quaternion normalised = q1.getNormalised();

    EXPECT_DOUBLE_EQ(normalised.x(), 0.2100420126042014) << "Quaternion normalisation failed on x";
    EXPECT_DOUBLE_EQ(normalised.y(), 0.3500700210070024) << "Quaternion normalisation failed on y";
    EXPECT_DOUBLE_EQ(normalised.z(), 0.4900980294098034) << "Quaternion normalisation failed on z";
    EXPECT_DOUBLE_EQ(normalised.w(), 0.7701540462154052) << "Quaternion normalisation failed on w";
}

TEST(MathTest, QuaternionTransformPoint)
{
    Quaternion q1(3, 5, 7, 11);
    Vector3 point(13, 17, 19);

    Vector3 transformed = q1.transformPoint(point);

    EXPECT_EQ(transformed.x(), q1.w() * q1.w() * point.x() + 2 * q1.y() * q1.w() * point.z() - 2 * q1.z() * q1.w() * point.y() + q1.x() * q1.x() * point.x() + 2 * q1.y() * q1.x() * point.y() + 2 * q1.z() * q1.x() * point.z() - q1.z() * q1.z() * point.x() - q1.y() * q1.y() * point.x()) << "Quaternion point transformation failed on x";
    EXPECT_EQ(transformed.y(), 2 * q1.x() * q1.y() * point.x() + q1.y() * q1.y() * point.y() + 2 * q1.z() * q1.y() * point.z() + 2 * q1.w() * q1.z() * point.x() - q1.z() * q1.z() * point.y() + q1.w() * q1.w() * point.y() - 2 * q1.x() * q1.w() * point.z() - q1.x() * q1.x() * point.y()) << "Quaternion point transformation failed on y";
    EXPECT_EQ(transformed.z(), 2 * q1.x() * q1.z() * point.x() + 2 * q1.y() * q1.z() * point.y() + q1.z() * q1.z() * point.z() - 2 * q1.w() * q1.y() * point.x() - q1.y() * q1.y() * point.z() + 2 * q1.w() * q1.x() * point.y() - q1.x() * q1.x() * point.z() + q1.w() * q1.w() * point.z()) << "Quaternion point transformation failed on z";
}

}
