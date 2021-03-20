#include "gtest/gtest.h"

#include "math/Matrix4.h"

namespace test
{

namespace
{
    double angle = 30.0;

    double cosAngle = cos(degrees_to_radians(angle));
    double sinAngle = sin(degrees_to_radians(angle));
    double EPSILON = 0.00001f;
}

TEST(MathTest, IdentityMatrix)
{
    const Matrix4 identity = Matrix4::getIdentity();

    EXPECT_EQ(identity.xx(), 1) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.xy(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.xz(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.xw(), 0) << "Wrong values in identity matrix";

    EXPECT_EQ(identity.yx(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.yy(), 1) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.yz(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.yw(), 0) << "Wrong values in identity matrix";

    EXPECT_EQ(identity.zx(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.zy(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.zz(), 1) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.zw(), 0) << "Wrong values in identity matrix";

    EXPECT_EQ(identity.tx(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.ty(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.tz(), 0) << "Wrong values in identity matrix";
    EXPECT_EQ(identity.tw(), 1) << "Wrong values in identity matrix";

    Matrix4 identity2;

    identity2.xx() = 1;
    identity2.xy() = 0;
    identity2.xz() = 0;
    identity2.xw() = 0;

    identity2.yx() = 0;
    identity2.yy() = 1;
    identity2.yz() = 0;
    identity2.yw() = 0;

    identity2.zx() = 0;
    identity2.zy() = 0;
    identity2.zz() = 1;
    identity2.zw() = 0;

    identity2.tx() = 0;
    identity2.ty() = 0;
    identity2.tz() = 0;
    identity2.tw() = 1;

    EXPECT_TRUE(identity == identity2) << "Explicitly constructed identity not equal to Matrix4::getIdentity()";
}

TEST(MathTest, ConstructMatrixByRows)
{
    auto m = Matrix4::byRows(1, 2.5, 3, 0.34,
        51, -6, 7, 9,
        9, 100, 11, 20,
        13.11, 24, 15, 32);

    // Check individual values
    EXPECT_EQ(m.xx(), 1);
    EXPECT_EQ(m.xy(), 51);
    EXPECT_EQ(m.xz(), 9);
    EXPECT_EQ(m.xw(), 13.11);

    EXPECT_EQ(m.yx(), 2.5);
    EXPECT_EQ(m.yy(), -6);
    EXPECT_EQ(m.yz(), 100);
    EXPECT_EQ(m.yw(), 24);

    EXPECT_EQ(m.zx(), 3);
    EXPECT_EQ(m.zy(), 7);
    EXPECT_EQ(m.zz(), 11);
    EXPECT_EQ(m.zw(), 15);

    EXPECT_EQ(m.tx(), 0.34);
    EXPECT_EQ(m.ty(), 9);
    EXPECT_EQ(m.tz(), 20);
    EXPECT_EQ(m.tw(), 32);

    // Check vector components
    EXPECT_EQ(m.x(), Vector4(1, 51, 9, 13.11));
    EXPECT_EQ(m.y(), Vector4(2.5, -6, 100, 24));
    EXPECT_EQ(m.z(), Vector4(3, 7, 11, 15));
    EXPECT_EQ(m.t(), Vector4(0.34, 9, 20, 32));
    EXPECT_EQ(m.translation(), Vector3(0.34, 9, 20));
}

TEST(MathTest, MatrixRotationAboutXDegrees)
{
    double angle = 30.0;
    double cosAngle = cos(degrees_to_radians(angle));
    double sinAngle = sin(degrees_to_radians(angle));

    // Test X rotation
    auto xRot = Matrix4::getRotationAboutXDegrees(angle);

    EXPECT_DOUBLE_EQ(xRot.xx(), 1) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.xy(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.xz(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.xw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(xRot.yx(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.yy(), cosAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.yz(), sinAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.yw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(xRot.zx(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.zy(), -sinAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.zz(), cosAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.zw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(xRot.tx(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.ty(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.tz(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(xRot.tw(), 1) << "Matrix rotation constructor failed";
}

TEST(MathTest, MatrixRotationAboutYDegrees)
{
    double angle = 30.0;
    double cosAngle = cos(degrees_to_radians(angle));
    double sinAngle = sin(degrees_to_radians(angle));

    // Test Y rotation
    auto yRot = Matrix4::getRotationAboutYDegrees(angle);

    EXPECT_DOUBLE_EQ(yRot.xx(), cosAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.xy(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.xz(), -sinAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.xw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(yRot.yx(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.yy(), 1) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.yz(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.yw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(yRot.zx(), sinAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.zy(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.zz(), cosAngle) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.zw(), 0) << "Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(yRot.tx(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.ty(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.tz(), 0) << "Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(yRot.tw(), 1) << "Matrix rotation constructor failed";
}

TEST(MathTest, MatrixRotationAboutZDegrees)
{
    double angle = 30.0;
    double cosAngle = cos(degrees_to_radians(angle));
    double sinAngle = sin(degrees_to_radians(angle));

    // Test Z rotation
    auto zRot = Matrix4::getRotationAboutZDegrees(angle);

    EXPECT_DOUBLE_EQ(zRot.xx(), cosAngle) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.xy(), sinAngle) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.xz(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.xw(), 0) <<"Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(zRot.yx(), -sinAngle) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.yy(), cosAngle) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.yz(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.yw(), 0) <<"Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(zRot.zx(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.zy(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.zz(), 1) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.zw(), 0) <<"Matrix rotation constructor failed";

    EXPECT_DOUBLE_EQ(zRot.tx(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.ty(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.tz(), 0) <<"Matrix rotation constructor failed";
    EXPECT_DOUBLE_EQ(zRot.tw(), 1) <<"Matrix rotation constructor failed";
}

TEST(MathTest, MatrixRotationForEulerXYZDegrees)
{
    // Test euler angle constructors
    Vector3 euler(30, -55, 75);

    // Convert degrees to radians
    double pi = 3.141592653589793238462643383f;
    double cx = cos(euler[0] * c_pi / 180.0f);
    double sx = sin(euler[0] * c_pi / 180.0f);
    double cy = cos(euler[1] * c_pi / 180.0f);
    double sy = sin(euler[1] * c_pi / 180.0f);
    double cz = cos(euler[2] * c_pi / 180.0f);
    double sz = sin(euler[2] * c_pi / 180.0f);

    Matrix4 eulerXYZ = Matrix4::getRotationForEulerXYZDegrees(euler);

    EXPECT_DOUBLE_EQ(eulerXYZ.xx(), cy * cz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.xy(), cy * sz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.xz(), -sy) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.xw(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerXYZ.yx(), sx * sy * cz - cx * sz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.yy(), sx * sy * sz + cx * cz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.yz(), sx * cy) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.yw(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerXYZ.zx(), cx * sy * cz + sx * sz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.zy(), cx * sy * sz - sx * cz) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.zz(), cx * cy) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.zw(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerXYZ.tx(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.ty(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.tz(), 0) <<"Matrix getRotationForEulerXYZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerXYZ.tw(), 1) <<"Matrix getRotationForEulerXYZDegrees failed";

    // Test Euler Angle retrieval (XYZ)
    Vector3 testEuler = eulerXYZ.getEulerAnglesXYZDegrees();

    EXPECT_DOUBLE_EQ(testEuler.x(), euler.x()) << "getEulerAnglesXYZDegrees fault at x()";
    EXPECT_DOUBLE_EQ(testEuler.y(), euler.y()) << "getEulerAnglesXYZDegrees fault at y()";
    EXPECT_DOUBLE_EQ(testEuler.z(), euler.z()) << "getEulerAnglesXYZDegrees fault at z()";
}

TEST(MathTest, MatrixRotationForEulerYXZDegrees)
{
    // Test euler angle constructors
    Vector3 euler(30, -55, 75);

    // Convert degrees to radians
    double pi = 3.141592653589793238462643383f;
    double cx = cos(euler[0] * c_pi / 180.0f);
    double sx = sin(euler[0] * c_pi / 180.0f);
    double cy = cos(euler[1] * c_pi / 180.0f);
    double sy = sin(euler[1] * c_pi / 180.0f);
    double cz = cos(euler[2] * c_pi / 180.0f);
    double sz = sin(euler[2] * c_pi / 180.0f);

    Matrix4 eulerYXZ = Matrix4::getRotationForEulerYXZDegrees(euler);

    EXPECT_DOUBLE_EQ(eulerYXZ.xx(), cy * cz - sx * sy * sz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.xy(), cy * sz + sx * sy * cz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.xz(), -cx * sy) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.xw(), 0) << "Matrix getRotationForEulerYXZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerYXZ.yx(), -cx * sz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.yy(), cx * cz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.yz(), sx) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.yw(), 0) << "Matrix getRotationForEulerYXZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerYXZ.zx(), sy * cz + sx * cy * sz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.zy(), sy * sz - sx * cy * cz) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.zz(), cx * cy) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.zw(), 0) << "Matrix getRotationForEulerYXZDegrees failed";

    EXPECT_DOUBLE_EQ(eulerYXZ.tx(), 0) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.ty(), 0) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.tz(), 0) << "Matrix getRotationForEulerYXZDegrees failed";
    EXPECT_DOUBLE_EQ(eulerYXZ.tw(), 1) << "Matrix getRotationForEulerYXZDegrees failed";

    // Test Euler Angle retrieval (YXZ)
    Vector3 testEuler = eulerYXZ.getEulerAnglesYXZDegrees();

    EXPECT_DOUBLE_EQ(testEuler.x(), euler.x()) << "getEulerAnglesYXZDegrees fault at x()";
    EXPECT_DOUBLE_EQ(testEuler.y(), euler.y()) << "getEulerAnglesYXZDegrees fault at y()";
    EXPECT_DOUBLE_EQ(testEuler.z(), euler.z()) << "getEulerAnglesYXZDegrees fault at z()";
}

TEST(MathTest, MatrixRotationForEulerZXYDegrees)
{
    // Test euler angle constructors
    Vector3 euler(30, -55, 75);

    // Convert degrees to radians
    double pi = 3.141592653589793238462643383f;
    double cx = cos(euler[0] * c_pi / 180.0f);
    double sx = sin(euler[0] * c_pi / 180.0f);
    double cy = cos(euler[1] * c_pi / 180.0f);
    double sy = sin(euler[1] * c_pi / 180.0f);
    double cz = cos(euler[2] * c_pi / 180.0f);
    double sz = sin(euler[2] * c_pi / 180.0f);

    Matrix4 eulerZXY = Matrix4::getRotationForEulerZXYDegrees(euler);

    EXPECT_DOUBLE_EQ(eulerZXY.xx(), cy * cz + sx * sy * sz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.xy(), cx * sz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.xz(), sx * cy * sz - sy * cz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.xw(), 0) << "Matrix getRotationForEulerZXYDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZXY.yx(), sx * sy * cz - cy * sz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.yy(), cx * cz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.yz(), sx * cy * cz + sy * sz) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.yw(), 0) << "Matrix getRotationForEulerZXYDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZXY.zx(), cx * sy) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.zy(), -sx) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.zz(), cx * cy) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.zw(), 0) << "Matrix getRotationForEulerZXYDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZXY.tx(), 0) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.ty(), 0) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.tz(), 0) << "Matrix getRotationForEulerZXYDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZXY.tw(), 1) << "Matrix getRotationForEulerZXYDegrees failed";

    // Test Euler Angle retrieval (ZXY)
    Vector3 testEuler = eulerZXY.getEulerAnglesZXYDegrees();

    EXPECT_DOUBLE_EQ(testEuler.x(), euler.x()) << "getEulerAnglesZXYDegrees fault at x()";
    EXPECT_DOUBLE_EQ(testEuler.y(), euler.y()) << "getEulerAnglesZXYDegrees fault at y()";
    EXPECT_DOUBLE_EQ(testEuler.z(), euler.z()) << "getEulerAnglesZXYDegrees fault at z()";
}

TEST(MathTest, MatrixRotationForEulerZYXDegrees)
{
    // Test euler angle constructors
    Vector3 euler(30, -55, 75);

    // Convert degrees to radians
    double pi = 3.141592653589793238462643383f;
    double cx = cos(euler[0] * c_pi / 180.0f);
    double sx = sin(euler[0] * c_pi / 180.0f);
    double cy = cos(euler[1] * c_pi / 180.0f);
    double sy = sin(euler[1] * c_pi / 180.0f);
    double cz = cos(euler[2] * c_pi / 180.0f);
    double sz = sin(euler[2] * c_pi / 180.0f);

    Matrix4 eulerZYX = Matrix4::getRotationForEulerZYXDegrees(euler);

    EXPECT_DOUBLE_EQ(eulerZYX.xx(), cy * cz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.xy(), cx * sz + sx * sy * cz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.xz(), sx * sz - cx * sy * cz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.xw(), 0) << "Matrix getRotationForEulerZYXDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZYX.yx(), -cy * sz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.yy(), cx * cz - sx * sy * sz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.yz(), sx * cz + cx * sy * sz) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.yw(), 0) << "Matrix getRotationForEulerZYXDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZYX.zx(), sy) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.zy(), -sx * cy) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.zz(), cx * cy) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.zw(), 0) << "Matrix getRotationForEulerZYXDegrees failed";

    EXPECT_DOUBLE_EQ(eulerZYX.tx(), 0) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.ty(), 0) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.tz(), 0) << "Matrix getRotationForEulerZYXDegrees failed";
    EXPECT_DOUBLE_EQ(eulerZYX.tw(), 1) << "Matrix getRotationForEulerZYXDegrees failed";

    // Test Euler Angle retrieval (ZYX)
    Vector3 testEuler = eulerZYX.getEulerAnglesZYXDegrees();

    EXPECT_DOUBLE_EQ(testEuler.x(), euler.x()) << "getEulerAnglesZYXDegrees fault at x()";
    EXPECT_DOUBLE_EQ(testEuler.y(), euler.y()) << "getEulerAnglesZYXDegrees fault at y()";
    EXPECT_DOUBLE_EQ(testEuler.z(), euler.z()) << "getEulerAnglesZYXDegrees fault at z()";
}

TEST(MathTest, MatrixMultiplication)
{
    auto a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);
    auto b = Matrix4::byColumns(61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137);

    auto c = a.getMultipliedBy(b);

    EXPECT_EQ(c.xx(), 6252) << "Matrix multiplication failed";
    EXPECT_EQ(c.xy(), 7076) << "Matrix multiplication failed";
    EXPECT_EQ(c.xz(), 8196) << "Matrix multiplication failed";
    EXPECT_EQ(c.xw(), 9430) << "Matrix multiplication failed";

    EXPECT_EQ(c.yx(), 8068) << "Matrix multiplication failed";
    EXPECT_EQ(c.yy(), 9124) << "Matrix multiplication failed";
    EXPECT_EQ(c.yz(), 10564) << "Matrix multiplication failed";
    EXPECT_EQ(c.yw(), 12150) << "Matrix multiplication failed";

    EXPECT_EQ(c.zx(), 9432) << "Matrix multiplication failed";
    EXPECT_EQ(c.zy(), 10696) << "Matrix multiplication failed";
    EXPECT_EQ(c.zz(), 12400) << "Matrix multiplication failed";
    EXPECT_EQ(c.zw(), 14298) << "Matrix multiplication failed";

    EXPECT_EQ(c.tx(), 11680) << "Matrix multiplication failed";
    EXPECT_EQ(c.ty(), 13224) << "Matrix multiplication failed";
    EXPECT_EQ(c.tz(), 15312) << "Matrix multiplication failed";
    EXPECT_EQ(c.tw(), 17618) << "Matrix multiplication failed";

    // Test Pre-Multiplication
    EXPECT_EQ(b.getMultipliedBy(a), a.getPremultipliedBy(b)) << "Matrix pre-multiplication mismatch";

    // Create an affine matrix
    Matrix4 affineA = a;

    affineA.xw() = 0;
    affineA.yw() = 0;
    affineA.zw() = 0;
    affineA.tw() = 1;

    EXPECT_TRUE(affineA.isAffine()) << "Affine check failed";
}

TEST(MathTest, MatrixTransformation)
{
    auto a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

    {
        Vector3 v(61, 67, 71);

        Vector3 transformed = a.transformPoint(v);

        EXPECT_EQ(transformed.x(), 3156) << "Vector3 transformation failed";
        EXPECT_EQ(transformed.y(), 3692) << "Vector3 transformation failed";
        EXPECT_EQ(transformed.z(), 4380) << "Vector3 transformation failed";

        Vector3 transformedDir = a.transformDirection(v);

        EXPECT_EQ(transformedDir.x(), 3113) << "Vector3 direction transformation failed";
        EXPECT_EQ(transformedDir.y(), 3645) << "Vector3 direction transformation failed";
        EXPECT_EQ(transformedDir.z(), 4327) << "Vector3 direction transformation failed";
    }

    {
        Vector4 vector(83, 89, 97, 101);
        Vector4 transformed = a.transform(vector);

        EXPECT_EQ(transformed.x(), 8562) << "Vector4 transformation failed";
        EXPECT_EQ(transformed.y(), 9682) << "Vector4 transformation failed";
        EXPECT_EQ(transformed.z(), 11214) << "Vector4 transformation failed";
        EXPECT_EQ(transformed.w(), 12896) << "Vector4 transformation failed";
    }

    EXPECT_EQ(a.t().x(), 43) << "Matrix4::t failed";
    EXPECT_EQ(a.t().y(), 47) << "Matrix4::t failed";
    EXPECT_EQ(a.t().z(), 53) << "Matrix4::t failed";
}

TEST(MathTest, MatrixScaleAffineInverse)
{
    // Construct a scale matrix
    Vector3 SCALE(2, 4, 8);
    Matrix4 scaleMat = Matrix4::getScale(SCALE);
    EXPECT_EQ(scaleMat.getScale(), SCALE);

    // Get the affine inverse
    Matrix4 inverse = scaleMat.getInverse();

    // Inverse must have inverted scale factors
    EXPECT_EQ(inverse.getScale(),
              Vector3(1.0 / SCALE.x(), 1.0 / SCALE.y(), 1.0 / SCALE.z()));
}

TEST(MathTest, MatrixTranslationAffineInverse)
{
    // Construct a translation matrix
    Vector3 TRANS(4, 32, -8);
    Matrix4 transMat = Matrix4::getTranslation(TRANS);
    EXPECT_EQ(transMat.getScale(), Vector3(1, 1, 1));

    // Get the affine inverse
    Matrix4 inverse = transMat.getInverse();

    // Check resulting matrix
    EXPECT_EQ(inverse, Matrix4::byRows(1, 0, 0, -TRANS.x(),
                                       0, 1, 0, -TRANS.y(),
                                       0, 0, 1, -TRANS.z(),
                                       0, 0, 0, 1));
}

TEST(MathTest, MatrixFullInverse)
{
    auto a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

    auto inv = a.getFullInverse();

    EXPECT_DOUBLE_EQ(inv.xx(), 0.39285714285714285) << "Matrix inversion failed on xx";
    EXPECT_DOUBLE_EQ(inv.xy(), -0.71428571428571419) << "Matrix inversion failed on xy";
    EXPECT_DOUBLE_EQ(inv.xz(), -0.3214285714285714) << "Matrix inversion failed on xz";
    EXPECT_DOUBLE_EQ(inv.xw(), 0.42857142857142855) << "Matrix inversion failed on xw";

    EXPECT_DOUBLE_EQ(inv.yx(), -0.27678571428571425) << "Matrix inversion failed on yx";
    EXPECT_DOUBLE_EQ(inv.yy(), 0.4464285714285714) << "Matrix inversion failed on yy";
    EXPECT_DOUBLE_EQ(inv.yz(), -0.33035714285714285) << "Matrix inversion failed on yz";
    EXPECT_DOUBLE_EQ(inv.yw(), 0.10714285714285714) << "Matrix inversion failed on yw";

    EXPECT_DOUBLE_EQ(inv.zx(), -0.6696428571428571) << "Matrix inversion failed on zx";
    EXPECT_DOUBLE_EQ(inv.zy(), 0.6607142857142857) << "Matrix inversion failed on zy";
    EXPECT_DOUBLE_EQ(inv.zz(), 0.99107142857142849) << "Matrix inversion failed on zz";
    EXPECT_DOUBLE_EQ(inv.zw(), -0.8214285714285714) << "Matrix inversion failed on zw";

    EXPECT_DOUBLE_EQ(inv.tx(), 0.5357142857142857) << "Matrix inversion failed on tx";
    EXPECT_DOUBLE_EQ(inv.ty(), -0.42857142857142855) << "Matrix inversion failed on ty";
    EXPECT_DOUBLE_EQ(inv.tz(), -0.39285714285714285) << "Matrix inversion failed on tz";
    EXPECT_DOUBLE_EQ(inv.tw(), 0.3571428571428571) << "Matrix inversion failed on tw";
}

}
