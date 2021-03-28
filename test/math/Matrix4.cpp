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

    // EXPECT that two matrices are close to each other (using GTest's own
    // EXPECT_DOUBLE_EQ)
    void expectNear(const Matrix4& m1, const Matrix4& m2)
    {
        EXPECT_DOUBLE_EQ(m1.xx(), m2.xx());
        EXPECT_DOUBLE_EQ(m1.xy(), m2.xy());
        EXPECT_DOUBLE_EQ(m1.xz(), m2.xz());
        EXPECT_DOUBLE_EQ(m1.xw(), m2.xw());

        EXPECT_DOUBLE_EQ(m1.yx(), m2.yx());
        EXPECT_DOUBLE_EQ(m1.yy(), m2.yy());
        EXPECT_DOUBLE_EQ(m1.yz(), m2.yz());
        EXPECT_DOUBLE_EQ(m1.yw(), m2.yw());

        EXPECT_DOUBLE_EQ(m1.zx(), m2.zx());
        EXPECT_DOUBLE_EQ(m1.zy(), m2.zy());
        EXPECT_DOUBLE_EQ(m1.zz(), m2.zz());
        EXPECT_DOUBLE_EQ(m1.zw(), m2.zw());

        EXPECT_DOUBLE_EQ(m1.tx(), m2.tx());
        EXPECT_DOUBLE_EQ(m1.ty(), m2.ty());
        EXPECT_DOUBLE_EQ(m1.tz(), m2.tz());
        EXPECT_DOUBLE_EQ(m1.tw(), m2.tw());
    }
}

TEST(MathTest, CreateIdentityMatrix)
{
    const Matrix4 identity = Matrix4::getIdentity();
    EXPECT_EQ(identity, Matrix4::byRows(1, 0, 0, 0,
                                        0, 1, 0, 0,
                                        0, 0, 1, 0,
                                        0, 0, 0, 1));
}

TEST(MathTest, AssignMatrixComponents)
{
    Matrix4 identity;

    identity.xx() = 1;
    identity.xy() = 0;
    identity.xz() = 0;
    identity.xw() = 0;

    identity.yx() = 0;
    identity.yy() = 1;
    identity.yz() = 0;
    identity.yw() = 0;

    identity.zx() = 0;
    identity.zy() = 0;
    identity.zz() = 1;
    identity.zw() = 0;

    identity.tx() = 0;
    identity.ty() = 0;
    identity.tz() = 0;
    identity.tw() = 1;

    EXPECT_EQ(identity, Matrix4::getIdentity());
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
}

TEST(MathTest, AccessMatrixColumnVectors)
{
    Matrix4 m = Matrix4::byRows(1, 4, 8, -5,
                                2, 9, 7, 13,
                                11, -2, 10, 14,
                                26, -100, 0.5, 3);

    // Read column values
    EXPECT_EQ(m.xCol(), Vector4(1, 2, 11, 26));
    EXPECT_EQ(m.yCol(), Vector4(4, 9, -2, -100));
    EXPECT_EQ(m.zCol(), Vector4(8, 7, 10, 0.5));
    EXPECT_EQ(m.tCol(), Vector4(-5, 13, 14, 3));
    EXPECT_EQ(m.translation(), Vector3(-5, 13, 14));

    // Write column values
    m.xCol() = Vector4(0.1, 0.2, 0.3, 0.4);
    m.yCol() = Vector4(0.5, 0.6, 0.7, 0.8);
    m.zCol() = Vector4(0.9, 1.0, 1.1, 1.2);
    m.tCol() = Vector4(1.3, 1.4, 1.5, 1.6);
    EXPECT_EQ(m, Matrix4::byColumns(0.1, 0.2, 0.3, 0.4,
                                    0.5, 0.6, 0.7, 0.8,
                                    0.9, 1.0, 1.1, 1.2,
                                    1.3, 1.4, 1.5, 1.6));
}

TEST(MathTest, MatrixRawArrayData)
{
    Matrix4 m = Matrix4::byRows(1, 0.2, 35, 4,
                                5, -6, 17, 300,
                                3.02, 10, -11, 12.4,
                                20, 17, 3001, -4.5);

    // Confirm data can be accessed as a packed C array of doubles (e.g. for
    // OpenGL methods)
    double* data = static_cast<double*>(m);
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[1], 5);
    EXPECT_EQ(data[2], 3.02);
    EXPECT_EQ(data[3], 20);
    EXPECT_EQ(data[4], 0.2);
    EXPECT_EQ(data[5], -6);
    EXPECT_EQ(data[6], 10);
    EXPECT_EQ(data[7], 17);
    EXPECT_EQ(data[8], 35);
    EXPECT_EQ(data[9], 17);
    EXPECT_EQ(data[10], -11);
    EXPECT_EQ(data[11], 3001);
    EXPECT_EQ(data[12], 4);
    EXPECT_EQ(data[13], 300);
    EXPECT_EQ(data[14], 12.4);
    EXPECT_EQ(data[15], -4.5);
}

TEST(MathTest, MatrixEquality)
{
    Matrix4 m1 = Matrix4::byRows(1, 2, 3.5, 4,
                                 5, -6, 17, 800,
                                 9.01, 10, 11, 12.4,
                                 200, -10, 300, 400);
    Matrix4 m2 = m1;
    EXPECT_TRUE(m1 == m2);
    EXPECT_EQ(m1, m2);
    EXPECT_TRUE(m1 != Matrix4::getIdentity());
    EXPECT_TRUE(m2 != Matrix4::getIdentity());
}

TEST(MathTest, MatrixTranspose)
{
    Matrix4 m = Matrix4::byRows(1, 2, 3, 4,
                                5, 6, 7, 8,
                                9, 10, 11, 12,
                                13, 14, 15, 16);
    Matrix4 mT = Matrix4::byRows(1, 5, 9, 13,
                                 2, 6, 10, 14,
                                 3, 7, 11, 15,
                                 4, 8, 12, 16);

    // Return transposed copy
    EXPECT_EQ(m.getTransposed(), mT);

    // Transpose in place
    EXPECT_NE(m, mT);
    m.transpose();
    EXPECT_EQ(m, mT);
}

TEST(MathTest, ConvertDegreesAndRadians)
{
    math::Degrees thirtyD(30);
    EXPECT_DOUBLE_EQ(thirtyD.asDegrees(), 30);
    EXPECT_DOUBLE_EQ(thirtyD.asRadians(), math::PI / 6.0);

    math::Radians twoPiBy3R(2 * math::PI / 3.0);
    EXPECT_DOUBLE_EQ(twoPiBy3R.asDegrees(), 120);
    EXPECT_DOUBLE_EQ(twoPiBy3R.asRadians(), 2 * math::PI / 3.0);
}

TEST(MathTest, MatrixRotationAboutZDegrees)
{
    math::Degrees angle(60.0);
    double cosAngle = cos(angle.asRadians());
    double sinAngle = sin(angle.asRadians());

    // Test Z rotation
    auto zRot = Matrix4::getRotationAboutZ(angle);
    expectNear(zRot, Matrix4::byRows(cosAngle, -sinAngle, 0, 0,
                                     sinAngle, cosAngle, 0, 0,
                                     0, 0, 1, 0,
                                     0, 0, 0, 1));
}

TEST(MathTest, MatrixRotationAboutZRadians)
{
    double angle = math::PI / 3.0; // 60 degrees in radians
    double cosAngle = cos(angle);
    double sinAngle = sin(angle);

    // Test Z rotation
    auto zRot = Matrix4::getRotationAboutZ(math::Radians(angle));
    expectNear(zRot, Matrix4::byRows(cosAngle, -sinAngle, 0, 0,
                                     sinAngle, cosAngle, 0, 0,
                                     0, 0, 1, 0,
                                     0, 0, 0, 1));
}

TEST(MathTest, MatrixRotationForEulerXYZDegrees)
{
    // Test euler angle constructors
    Vector3 euler(30, -55, 75);

    // Convert degrees to radians
    double pi = 3.141592653589793238462643383f;
    double cx = cos(euler[0] * math::PI / 180.0f);
    double sx = sin(euler[0] * math::PI / 180.0f);
    double cy = cos(euler[1] * math::PI / 180.0f);
    double sy = sin(euler[1] * math::PI / 180.0f);
    double cz = cos(euler[2] * math::PI / 180.0f);
    double sz = sin(euler[2] * math::PI / 180.0f);

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

TEST(MathTest, MatrixMultiplication)
{
    auto a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);
    auto b = Matrix4::byColumns(61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137);

    // Check multiplied result
    auto c = a.getMultipliedBy(b);
    EXPECT_EQ(c, Matrix4::byColumns(6252, 7076, 8196, 9430,
                                    8068, 9124, 10564, 12150,
                                    9432, 10696, 12400, 14298,
                                    11680, 13224, 15312, 17618));

    // Multiplication has not changed original
    EXPECT_NE(a, c);

    // Check operator multiplication as well
    EXPECT_EQ(a * b, c);

    // Test Pre-Multiplication
    EXPECT_EQ(b.getMultipliedBy(a), a.getPremultipliedBy(b)) << "Matrix pre-multiplication mismatch";
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

    EXPECT_EQ(a.tCol().x(), 43) << "Matrix4::t failed";
    EXPECT_EQ(a.tCol().y(), 47) << "Matrix4::t failed";
    EXPECT_EQ(a.tCol().z(), 53) << "Matrix4::t failed";
}

TEST(MathTest, MatrixTransformVectorEqualsMultiplication)
{
    const Vector3 TRANSLATION(2, 5, -7);
    auto t = Matrix4::getTranslation(TRANSLATION)
           * Matrix4::getRotation(Vector3(0, 1, 0), 2.1)
           * Matrix4::getScale(Vector3(2, 2.5, 0.75));

    // transformPoint must be equivalent to a multiplication
    Vector3 vec3(16, -32, 0.05);
    EXPECT_EQ(t * vec3, t.transformPoint(vec3));

    // transformDirection does NOT give the same result, because it ignores
    // translation (vector's W coordinate is assumed 0).
    EXPECT_NE(t * vec3, t.transformDirection(vec3));

    // If we transform the direction then add the translation manually, we
    // should get the same result as transformPoint.
    EXPECT_EQ(t * vec3, t.transformDirection(vec3) + TRANSLATION);

    // transform() must be equivalent to a multiplication with a full Vector4
    Vector4 vec4(128, 0.025, -8198, 0.5);
    EXPECT_EQ(t * vec4, t.transform(vec4));

    // transformPoint() and transformDirection() must be equivalent to a
    // multiplication with the appropriate Vector4 (W == 0 or W == 1).
    EXPECT_EQ((t * Vector4(vec3, 1)).getVector3(), t.transformPoint(vec3));
    EXPECT_EQ((t * Vector4(vec3, 0)).getVector3(), t.transformDirection(vec3));
}

TEST(MathTest, MatrixScaleAffineInverse)
{
    // Construct a scale matrix
    Vector3 SCALE(2, 4, 8);
    Matrix4 scaleMat = Matrix4::getScale(SCALE);
    EXPECT_EQ(scaleMat.getScale(), SCALE);

    // Get the affine inverse
    Matrix4 inverse = scaleMat.getInverse();
    EXPECT_NE(inverse, scaleMat);
    scaleMat.invert();
    EXPECT_EQ(scaleMat, inverse);

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
    EXPECT_NE(inverse, transMat);
    transMat.invert();
    EXPECT_EQ(transMat, inverse);

    // Check resulting matrix
    EXPECT_EQ(inverse, Matrix4::byRows(1, 0, 0, -TRANS.x(),
                                       0, 1, 0, -TRANS.y(),
                                       0, 0, 1, -TRANS.z(),
                                       0, 0, 0, 1));
}

TEST(MathTest, MatrixRotationAffineInverse)
{
    // Construct a translation matrix
    const math::Degrees ANGLE(60);
    Matrix4 rotMat = Matrix4::getRotationAboutZ(ANGLE);

    // Get the affine inverse
    Matrix4 inverse = rotMat.getInverse();

    // Inverse should be a rotation in the other direction
    const math::Degrees REV_ANGLE(-60);
    Matrix4 backRotMat = Matrix4::getRotationAboutZ(REV_ANGLE);
    expectNear(inverse, backRotMat);
}

TEST(MathTest, MatrixAffineInverseMatchesFullInverse)
{
    // Create an affine transformation
    Matrix4 affTrans = Matrix4::getRotationAboutZ(math::Degrees(78))
                     * Matrix4::getScale(Vector3(2, 0.5, 1.2))
                     * Matrix4::getTranslation(Vector3(50, -8, 0));

    // Since this is an affine transformation, the inverse should be the same as
    // the affine inverse
    expectNear(affTrans.getInverse(), affTrans.getFullInverse());

    // Make a non-affine transformation
    Matrix4 nonAffTrans = affTrans;
    nonAffTrans.xw() = 0.5;
    nonAffTrans.tw() = 2;

    // This time the affine inverse and full inverse should be different.
    // Inspect a couple of values to make sure (there is no convenient assertion
    // for "two matrices are NOT nearly equal", but we can subtract them and
    // check that the differences exceed some threshold)
    Matrix4 aInv = nonAffTrans.getInverse();
    Matrix4 fInv = nonAffTrans.getFullInverse();
    Matrix4 diffInv = aInv - fInv;
    EXPECT_GT(diffInv.xx(), 0.1);
    EXPECT_GT(diffInv.yx(), 0.5);
    EXPECT_GT(diffInv.ty(), 8);
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
