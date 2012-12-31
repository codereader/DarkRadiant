#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE matrixTest
#include <boost/test/unit_test.hpp>

#include <math/Matrix4.h>

BOOST_AUTO_TEST_CASE(constructIdentity)
{
    const Matrix4 identity = Matrix4::getIdentity();

    BOOST_CHECK(identity.xx() == 1);
    BOOST_CHECK(identity.xy() == 0);
    BOOST_CHECK(identity.xz() == 0);
    BOOST_CHECK(identity.xw() == 0);

    BOOST_CHECK(identity.yx() == 0);
    BOOST_CHECK(identity.yy() == 1);
    BOOST_CHECK(identity.yz() == 0);
    BOOST_CHECK(identity.yw() == 0);

    BOOST_CHECK(identity.zx() == 0);
    BOOST_CHECK(identity.zy() == 0);
    BOOST_CHECK(identity.zz() == 1);
    BOOST_CHECK(identity.zw() == 0);

    BOOST_CHECK(identity.tx() == 0);
    BOOST_CHECK(identity.ty() == 0);
    BOOST_CHECK(identity.tz() == 0);
    BOOST_CHECK(identity.tw() == 1);

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

    BOOST_CHECK(identity == identity2);
}

BOOST_AUTO_TEST_CASE(constructInitialised)
{
    Matrix4 m = Matrix4::byRows(1,     2.5, 3,  0.34,
                                51,    -6,  7,  9,
                                9,     100, 11, 20,
                                13.11, 24,  15, 32);

    // Check individual values
    BOOST_CHECK_EQUAL(m.xx(), 1);
    BOOST_CHECK_EQUAL(m.xy(), 51);
    BOOST_CHECK_EQUAL(m.xz(), 9);
    BOOST_CHECK_EQUAL(m.xw(), 13.11);

    BOOST_CHECK_EQUAL(m.yx(), 2.5);
    BOOST_CHECK_EQUAL(m.yy(), -6);
    BOOST_CHECK_EQUAL(m.yz(), 100);
    BOOST_CHECK_EQUAL(m.yw(), 24);

    BOOST_CHECK_EQUAL(m.zx(), 3);
    BOOST_CHECK_EQUAL(m.zy(), 7);
    BOOST_CHECK_EQUAL(m.zz(), 11);
    BOOST_CHECK_EQUAL(m.zw(), 15);

    BOOST_CHECK_EQUAL(m.tx(), 0.34);
    BOOST_CHECK_EQUAL(m.ty(), 9);
    BOOST_CHECK_EQUAL(m.tz(), 20);
    BOOST_CHECK_EQUAL(m.tw(), 32);

    // Check vector components
    BOOST_CHECK_EQUAL(m.x(), Vector4(1, 51, 9, 13.11));
    BOOST_CHECK_EQUAL(m.y(), Vector4(2.5, -6, 100, 24));
    BOOST_CHECK_EQUAL(m.z(), Vector4(3, 7, 11, 15));
    BOOST_CHECK_EQUAL(m.t(), Vector4(0.34, 9, 20, 32));
    BOOST_CHECK_EQUAL(m.translation(), Vector3(0.34, 9, 20));
}

BOOST_AUTO_TEST_CASE(testRotationMatrices)
{
    double angle = 30.0;

    double cosAngle = cos(degrees_to_radians(angle));
    double sinAngle = sin(degrees_to_radians(angle));
    double EPSILON = 0.00001f;

    // Test X rotation
    Matrix4 xRot = Matrix4::getRotationAboutXDegrees(angle);

    BOOST_CHECK(float_equal_epsilon(xRot.xx(), 1, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.xy(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.xz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.xw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(xRot.yx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.yy(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.yz(), sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.yw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(xRot.zx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.zy(), -sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.zz(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.zw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(xRot.tx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.ty(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.tz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(xRot.tw(), 1, EPSILON));

    // Test Y rotation
    Matrix4 yRot = Matrix4::getRotationAboutYDegrees(angle);

    BOOST_CHECK(float_equal_epsilon(yRot.xx(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.xy(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.xz(), -sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.xw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(yRot.yx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.yy(), 1, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.yz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.yw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(yRot.zx(), sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.zy(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.zz(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.zw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(yRot.tx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.ty(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.tz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(yRot.tw(), 1, EPSILON));

    // Test Z rotation
    Matrix4 zRot = Matrix4::getRotationAboutZDegrees(angle);

    BOOST_CHECK(float_equal_epsilon(zRot.xx(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.xy(), sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.xz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.xw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(zRot.yx(), -sinAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.yy(), cosAngle, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.yz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.yw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(zRot.zx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.zy(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.zz(), 1, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.zw(), 0, EPSILON));

    BOOST_CHECK(float_equal_epsilon(zRot.tx(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.ty(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.tz(), 0, EPSILON));
    BOOST_CHECK(float_equal_epsilon(zRot.tw(), 1, EPSILON));

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

    // XYZ
    {
        Matrix4 eulerXYZ = Matrix4::getRotationForEulerXYZDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerXYZ.xx(), cy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.xy(), cy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.xz(), -sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXYZ.yx(), sx*sy*cz - cx*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.yy(), sx*sy*sz + cx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.yz(), sx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXYZ.zx(), cx*sy*cz + sx*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.zy(), cx*sy*sz - sx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.zz(), cx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXYZ.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXYZ.tw(), 1, EPSILON));
    }

    // YZX
    {
        Matrix4 eulerYZX = Matrix4::getRotationForEulerYZXDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerYZX.xx(), cy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.xy(), cx*cy*sz + sx*sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.xz(), sx*cy*sz - cx*sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYZX.yx(), -sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.yy(), cx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.yz(), sx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYZX.zx(), sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.zy(), cx*sy*sz - sx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.zz(), sx*sy*sz + cx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYZX.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYZX.tw(), 1, EPSILON));
    }

    // XZY
    {
        Matrix4 eulerXZY = Matrix4::getRotationForEulerXZYDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerXZY.xx(), cy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.xy(), sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.xz(), -sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXZY.yx(), sx*sy - cx*cy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.yy(), cx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.yz(), cx*sy*sz + sx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXZY.zx(), sx*cy*sz + cx*sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.zy(), -sx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.zz(), cx*cy - sx*sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerXZY.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerXZY.tw(), 1, EPSILON));
    }

    // YXZ
    {
        Matrix4 eulerYXZ = Matrix4::getRotationForEulerYXZDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerYXZ.xx(), cy*cz - sx*sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.xy(), cy*sz + sx*sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.xz(), -cx*sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYXZ.yx(), -cx*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.yy(), cx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.yz(), sx, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYXZ.zx(), sy*cz + sx*cy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.zy(), sy*sz - sx*cy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.zz(), cx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerYXZ.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerYXZ.tw(), 1, EPSILON));
    }

    // ZXY
    {
        Matrix4 eulerZXY = Matrix4::getRotationForEulerZXYDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerZXY.xx(), cy*cz + sx*sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.xy(), cx*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.xz(), sx*cy*sz - sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZXY.yx(), sx*sy*cz - cy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.yy(), cx*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.yz(), sx*cy*cz + sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZXY.zx(), cx*sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.zy(), -sx, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.zz(), cx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZXY.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZXY.tw(), 1, EPSILON));
    }

    // ZYX
    {
        Matrix4 eulerZYX = Matrix4::getRotationForEulerZYXDegrees(euler);

        BOOST_CHECK(float_equal_epsilon(eulerZYX.xx(), cy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.xy(), cx*sz + sx*sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.xz(), sx*sz - cx*sy*cz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.xw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZYX.yx(), -cy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.yy(), cx*cz - sx*sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.yz(), sx*cz + cx*sy*sz, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.yw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZYX.zx(), sy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.zy(), -sx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.zz(), cx*cy, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.zw(), 0, EPSILON));

        BOOST_CHECK(float_equal_epsilon(eulerZYX.tx(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.ty(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.tz(), 0, EPSILON));
        BOOST_CHECK(float_equal_epsilon(eulerZYX.tw(), 1, EPSILON));
    }

    // Test Euler Angle retrieval (XYZ)
    {
        Matrix4 eulerXYZ = Matrix4::getRotationForEulerXYZDegrees(euler);

        Vector3 testEuler = eulerXYZ.getEulerAnglesXYZDegrees();

        BOOST_CHECK(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON));
    }

    // Test Euler Angle retrieval (YXZ)
    {
        Matrix4 eulerYXZ = Matrix4::getRotationForEulerYXZDegrees(euler);

        Vector3 testEuler = eulerYXZ.getEulerAnglesYXZDegrees();

        BOOST_CHECK(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON));
    }

    // Test Euler Angle retrieval (ZXY)
    {
        Matrix4 eulerZXY = Matrix4::getRotationForEulerZXYDegrees(euler);

        Vector3 testEuler = eulerZXY.getEulerAnglesZXYDegrees();

        BOOST_CHECK(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON));
    }

    // Test Euler Angle retrieval (ZYX)
    {
        Matrix4 eulerZYX = Matrix4::getRotationForEulerZYXDegrees(euler);

        Vector3 testEuler = eulerZYX.getEulerAnglesZYXDegrees();

        BOOST_CHECK(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON));
        BOOST_CHECK(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON));
    }
}

BOOST_AUTO_TEST_CASE(testMultiplication)
{
    Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);
    Matrix4 b = Matrix4::byColumns(61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137);

    Matrix4 c = a.getMultipliedBy(b);

    BOOST_CHECK(c.xx() == 6252);
    BOOST_CHECK(c.xy() == 7076);
    BOOST_CHECK(c.xz() == 8196);
    BOOST_CHECK(c.xw() == 9430);

    BOOST_CHECK(c.yx() == 8068);
    BOOST_CHECK(c.yy() == 9124);
    BOOST_CHECK(c.yz() == 10564);
    BOOST_CHECK(c.yw() == 12150);

    BOOST_CHECK(c.zx() == 9432);
    BOOST_CHECK(c.zy() == 10696);
    BOOST_CHECK(c.zz() == 12400);
    BOOST_CHECK(c.zw() == 14298);

    BOOST_CHECK(c.tx() == 11680);
    BOOST_CHECK(c.ty() == 13224);
    BOOST_CHECK(c.tz() == 15312);
    BOOST_CHECK(c.tw() == 17618);

    // Test Pre-Multiplication
    BOOST_CHECK(b.getMultipliedBy(a) == a.getPremultipliedBy(b));

    // Create an affine matrix
    Matrix4 affineA = a;

    affineA.xw() = 0;
    affineA.yw() = 0;
    affineA.zw() = 0;
    affineA.tw() = 1;

    BOOST_CHECK(affineA.isAffine());
}

BOOST_AUTO_TEST_CASE(testTransformation)
{
    Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

    {
        Vector3 v(61, 67, 71);

        Vector3 transformed = a.transformPoint(v);

        BOOST_CHECK(transformed.x() == 3156);
        BOOST_CHECK(transformed.y() == 3692);
        BOOST_CHECK(transformed.z() == 4380);

        Vector3 transformedDir = a.transformDirection(v);

        BOOST_CHECK(transformedDir.x() == 3113);
        BOOST_CHECK(transformedDir.y() == 3645);
        BOOST_CHECK(transformedDir.z() == 4327);
    }

    {
        Vector4 vector(83, 89, 97, 101);
        Vector4 transformed = a.transform(vector);

        BOOST_CHECK(transformed.x() == 8562);
        BOOST_CHECK(transformed.y() == 9682);
        BOOST_CHECK(transformed.z() == 11214);
        BOOST_CHECK(transformed.w() == 12896);
    }

    BOOST_CHECK_EQUAL(a.translation().x(), 43);
    BOOST_CHECK_EQUAL(a.translation().y(), 47);
    BOOST_CHECK_EQUAL(a.translation().z(), 53);
}

BOOST_AUTO_TEST_CASE(testMatrixDeterminant)
{
    Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

    BOOST_CHECK(a.getDeterminant() == -448);
}

BOOST_AUTO_TEST_CASE(testMatrixInversion)
{
    Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

    Matrix4 inv = a.getFullInverse();

    double EPSILON = 0.00001f;

    BOOST_CHECK(float_equal_epsilon(inv.xx(), 0.392857f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.xy(), -0.714286f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.xz(), -0.321429f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.xw(), 0.428571f, EPSILON));

    BOOST_CHECK(float_equal_epsilon(inv.yx(), -0.276786f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.yy(), 0.446429f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.yz(), -0.330357f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.yw(), 0.107143f, EPSILON));

    BOOST_CHECK(float_equal_epsilon(inv.zx(), -0.669643f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.zy(), 0.660714f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.zz(), 0.991071f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.zw(), -0.821429f, EPSILON));

    BOOST_CHECK(float_equal_epsilon(inv.tx(), 0.535714f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.ty(), -0.428571f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.tz(), -0.392857f, EPSILON));
    BOOST_CHECK(float_equal_epsilon(inv.tw(), 0.357143f, EPSILON));
}

