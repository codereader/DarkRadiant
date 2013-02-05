#include "MathTest.h"

#include "math/Matrix4.h"
#include "math/Quaternion.h"

void MathTest::run()
{
	testIdentity();
	testRotationMatrices();
	testMultiplication();
	testTransformation();
	testMatrixDeterminant();
	testMatrixInversion();
	testQuaternions();
}

void MathTest::testIdentity()
{
	const Matrix4 identity = Matrix4::getIdentity();

	REQUIRE_TRUE(identity.xx() == 1, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.xy() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.xz() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.xw() == 0, "Wrong values in identity matrix");

	REQUIRE_TRUE(identity.yx() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.yy() == 1, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.yz() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.yw() == 0, "Wrong values in identity matrix");

	REQUIRE_TRUE(identity.zx() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.zy() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.zz() == 1, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.zw() == 0, "Wrong values in identity matrix");

	REQUIRE_TRUE(identity.tx() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.ty() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.tz() == 0, "Wrong values in identity matrix");
	REQUIRE_TRUE(identity.tw() == 1, "Wrong values in identity matrix");

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

	REQUIRE_TRUE(identity == identity2, "Explicitly constructed identity not equal to Matrix4::getIdentity()");
}

void MathTest::testRotationMatrices()
{
	double angle = 30.0;

	double cosAngle = cos(degrees_to_radians(angle));
	double sinAngle = sin(degrees_to_radians(angle));
	double EPSILON = 0.00001f;

	// Test X rotation
	Matrix4 xRot = Matrix4::getRotationAboutXDegrees(angle);

	REQUIRE_TRUE(float_equal_epsilon(xRot.xx(), 1, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.xy(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.xz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.xw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(xRot.yx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.yy(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.yz(), sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.yw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(xRot.zx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.zy(), -sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.zz(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.zw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(xRot.tx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.ty(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.tz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(xRot.tw(), 1, EPSILON), "Matrix rotation constructor failed");

	// Test Y rotation
	Matrix4 yRot = Matrix4::getRotationAboutYDegrees(angle);

	REQUIRE_TRUE(float_equal_epsilon(yRot.xx(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.xy(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.xz(), -sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.xw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(yRot.yx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.yy(), 1, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.yz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.yw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(yRot.zx(), sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.zy(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.zz(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.zw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(yRot.tx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.ty(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.tz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(yRot.tw(), 1, EPSILON), "Matrix rotation constructor failed");

	// Test Z rotation
	Matrix4 zRot = Matrix4::getRotationAboutZDegrees(angle);

	REQUIRE_TRUE(float_equal_epsilon(zRot.xx(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.xy(), sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.xz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.xw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(zRot.yx(), -sinAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.yy(), cosAngle, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.yz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.yw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(zRot.zx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.zy(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.zz(), 1, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.zw(), 0, EPSILON), "Matrix rotation constructor failed");

	REQUIRE_TRUE(float_equal_epsilon(zRot.tx(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.ty(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.tz(), 0, EPSILON), "Matrix rotation constructor failed");
	REQUIRE_TRUE(float_equal_epsilon(zRot.tw(), 1, EPSILON), "Matrix rotation constructor failed");

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

		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.xx(), cy*cz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.xy(), cy*sz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.xz(), -sy, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.xw(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.yx(), sx*sy*cz - cx*sz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.yy(), sx*sy*sz + cx*cz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.yz(), sx*cy, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.yw(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.zx(), cx*sy*cz + sx*sz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.zy(), cx*sy*sz - sx*cz, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.zz(), cx*cy, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.zw(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.tx(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.ty(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.tz(), 0, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXYZ.tw(), 1, EPSILON), "Matrix getRotationForEulerXYZDegrees failed");
	}

	// YZX
	{
		Matrix4 eulerYZX = Matrix4::getRotationForEulerYZXDegrees(euler);

		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.xx(), cy*cz, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.xy(), cx*cy*sz + sx*sy, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.xz(), sx*cy*sz - cx*sy, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.xw(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.yx(), -sz, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.yy(), cx*cz, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.yz(), sx*cz, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.yw(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.zx(), sy*cz, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.zy(), cx*sy*sz - sx*cy, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.zz(), sx*sy*sz + cx*cy, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.zw(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.tx(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.ty(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.tz(), 0, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYZX.tw(), 1, EPSILON), "Matrix getRotationForEulerYZXDegrees failed");
	}

	// XZY
	{
		Matrix4 eulerXZY = Matrix4::getRotationForEulerXZYDegrees(euler);

		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.xx(), cy*cz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.xy(), sz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.xz(), -sy*cz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.xw(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.yx(), sx*sy - cx*cy*sz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.yy(), cx*cz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.yz(), cx*sy*sz + sx*cy, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.yw(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.zx(), sx*cy*sz + cx*sy, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.zy(), -sx*cz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.zz(), cx*cy - sx*sy*sz, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.zw(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.tx(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.ty(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.tz(), 0, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerXZY.tw(), 1, EPSILON), "Matrix getRotationForEulerXZYDegrees failed");
	}

	// YXZ
	{
		Matrix4 eulerYXZ = Matrix4::getRotationForEulerYXZDegrees(euler);

		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.xx(), cy*cz - sx*sy*sz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.xy(), cy*sz + sx*sy*cz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.xz(), -cx*sy, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.xw(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.yx(), -cx*sz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.yy(), cx*cz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.yz(), sx, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.yw(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.zx(), sy*cz + sx*cy*sz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.zy(), sy*sz - sx*cy*cz, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.zz(), cx*cy, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.zw(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.tx(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.ty(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.tz(), 0, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerYXZ.tw(), 1, EPSILON), "Matrix getRotationForEulerYXZDegrees failed");
	}

	// ZXY
	{
		Matrix4 eulerZXY = Matrix4::getRotationForEulerZXYDegrees(euler);

		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.xx(), cy*cz + sx*sy*sz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.xy(), cx*sz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.xz(), sx*cy*sz - sy*cz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.xw(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.yx(), sx*sy*cz - cy*sz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.yy(), cx*cz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.yz(), sx*cy*cz + sy*sz, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.yw(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.zx(), cx*sy, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.zy(), -sx, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.zz(), cx*cy, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.zw(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.tx(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.ty(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.tz(), 0, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZXY.tw(), 1, EPSILON), "Matrix getRotationForEulerZXYDegrees failed");
	}

	// ZYX
	{
		Matrix4 eulerZYX = Matrix4::getRotationForEulerZYXDegrees(euler);

		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.xx(), cy*cz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.xy(), cx*sz + sx*sy*cz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.xz(), sx*sz - cx*sy*cz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.xw(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.yx(), -cy*sz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.yy(), cx*cz - sx*sy*sz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.yz(), sx*cz + cx*sy*sz, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.yw(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.zx(), sy, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.zy(), -sx*cy, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.zz(), cx*cy, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.zw(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");

		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.tx(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.ty(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.tz(), 0, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
		REQUIRE_TRUE(float_equal_epsilon(eulerZYX.tw(), 1, EPSILON), "Matrix getRotationForEulerZYXDegrees failed");
	}

	// Test Euler Angle retrieval (XYZ)
	{
		Matrix4 eulerXYZ = Matrix4::getRotationForEulerXYZDegrees(euler);

		Vector3 testEuler = eulerXYZ.getEulerAnglesXYZDegrees();

		REQUIRE_TRUE(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON), "getEulerAnglesXYZDegrees fault at x()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON), "getEulerAnglesXYZDegrees fault at y()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON), "getEulerAnglesXYZDegrees fault at z()");
	}

	// Test Euler Angle retrieval (YXZ)
	{
		Matrix4 eulerYXZ = Matrix4::getRotationForEulerYXZDegrees(euler);

		Vector3 testEuler = eulerYXZ.getEulerAnglesYXZDegrees();

		REQUIRE_TRUE(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON), "getEulerAnglesYXZDegrees fault at x()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON), "getEulerAnglesYXZDegrees fault at y()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON), "getEulerAnglesYXZDegrees fault at z()");
	}

	// Test Euler Angle retrieval (ZXY)
	{
		Matrix4 eulerZXY = Matrix4::getRotationForEulerZXYDegrees(euler);

		Vector3 testEuler = eulerZXY.getEulerAnglesZXYDegrees();

		REQUIRE_TRUE(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON), "getEulerAnglesZXYDegrees fault at x()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON), "getEulerAnglesZXYDegrees fault at y()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON), "getEulerAnglesZXYDegrees fault at z()");
	}

	// Test Euler Angle retrieval (ZYX)
	{
		Matrix4 eulerZYX = Matrix4::getRotationForEulerZYXDegrees(euler);

		Vector3 testEuler = eulerZYX.getEulerAnglesZYXDegrees();
		
		REQUIRE_TRUE(float_equal_epsilon(testEuler.x(), euler.x(), EPSILON), "getEulerAnglesZYXDegrees fault at x()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.y(), euler.y(), EPSILON), "getEulerAnglesZYXDegrees fault at y()");
		REQUIRE_TRUE(float_equal_epsilon(testEuler.z(), euler.z(), EPSILON), "getEulerAnglesZYXDegrees fault at z()");
	}
}

void MathTest::testMultiplication()
{
	Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);
	Matrix4 b = Matrix4::byColumns(61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137);

	Matrix4 c = a.getMultipliedBy(b);
	
	REQUIRE_TRUE(c.xx() == 6252, "Matrix multiplication failed");
	REQUIRE_TRUE(c.xy() == 7076, "Matrix multiplication failed");
	REQUIRE_TRUE(c.xz() == 8196, "Matrix multiplication failed");
	REQUIRE_TRUE(c.xw() == 9430, "Matrix multiplication failed");

	REQUIRE_TRUE(c.yx() == 8068, "Matrix multiplication failed");
	REQUIRE_TRUE(c.yy() == 9124, "Matrix multiplication failed");
	REQUIRE_TRUE(c.yz() == 10564, "Matrix multiplication failed");
	REQUIRE_TRUE(c.yw() == 12150, "Matrix multiplication failed");

	REQUIRE_TRUE(c.zx() == 9432, "Matrix multiplication failed");
	REQUIRE_TRUE(c.zy() == 10696, "Matrix multiplication failed");
	REQUIRE_TRUE(c.zz() == 12400, "Matrix multiplication failed");
	REQUIRE_TRUE(c.zw() == 14298, "Matrix multiplication failed");

	REQUIRE_TRUE(c.tx() == 11680, "Matrix multiplication failed");
	REQUIRE_TRUE(c.ty() == 13224, "Matrix multiplication failed");
	REQUIRE_TRUE(c.tz() == 15312, "Matrix multiplication failed");
	REQUIRE_TRUE(c.tw() == 17618, "Matrix multiplication failed");

	// Test Pre-Multiplication
	REQUIRE_TRUE(b.getMultipliedBy(a) == a.getPremultipliedBy(b), "Matrix pre-multiplication mismatch");

	// Create an affine matrix
	Matrix4 affineA = a;

	affineA.xw() = 0;
	affineA.yw() = 0;
	affineA.zw() = 0;
	affineA.tw() = 1;

	REQUIRE_TRUE(affineA.isAffine(), "Affine check failed");
}

void MathTest::testTransformation()
{
	Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

	{
		Vector3 v(61, 67, 71);

		Vector3 transformed = a.transformPoint(v);

		REQUIRE_TRUE(transformed.x() == 3156, "Vector3 transformation failed");
		REQUIRE_TRUE(transformed.y() == 3692, "Vector3 transformation failed");
		REQUIRE_TRUE(transformed.z() == 4380, "Vector3 transformation failed");

		Vector3 transformedDir = a.transformDirection(v);

		REQUIRE_TRUE(transformedDir.x() == 3113, "Vector3 direction transformation failed");
		REQUIRE_TRUE(transformedDir.y() == 3645, "Vector3 direction transformation failed");
		REQUIRE_TRUE(transformedDir.z() == 4327, "Vector3 direction transformation failed");
	}

	{
		Vector4 vector(83, 89, 97, 101);
		Vector4 transformed = a.transform(vector);

		REQUIRE_TRUE(transformed.x() == 8562, "Vector4 transformation failed");
		REQUIRE_TRUE(transformed.y() == 9682, "Vector4 transformation failed");
		REQUIRE_TRUE(transformed.z() == 11214, "Vector4 transformation failed");
		REQUIRE_TRUE(transformed.w() == 12896, "Vector4 transformation failed");
	}

	REQUIRE_TRUE(a.t().x() == 43, "Matrix4::t failed");
	REQUIRE_TRUE(a.t().y() == 47, "Matrix4::t failed");
	REQUIRE_TRUE(a.t().z() == 53, "Matrix4::t failed");
}

void MathTest::testMatrixDeterminant()
{
	Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

	REQUIRE_TRUE(a.getDeterminant() == -448, "Matrix determinant calculation failed");
}

void MathTest::testMatrixInversion()
{
	Matrix4 a = Matrix4::byColumns(3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59);

	Matrix4 inv = a.getFullInverse();

	double EPSILON = 0.00001f;

	REQUIRE_TRUE(float_equal_epsilon(inv.xx(), 0.392857f, EPSILON), "Matrix inversion failed on xx");
	REQUIRE_TRUE(float_equal_epsilon(inv.xy(), -0.714286f, EPSILON), "Matrix inversion failed on xy");
	REQUIRE_TRUE(float_equal_epsilon(inv.xz(), -0.321429f, EPSILON), "Matrix inversion failed on xz");
	REQUIRE_TRUE(float_equal_epsilon(inv.xw(), 0.428571f, EPSILON), "Matrix inversion failed on xw");

	REQUIRE_TRUE(float_equal_epsilon(inv.yx(), -0.276786f, EPSILON), "Matrix inversion failed on yx");
	REQUIRE_TRUE(float_equal_epsilon(inv.yy(), 0.446429f, EPSILON), "Matrix inversion failed on yy");
	REQUIRE_TRUE(float_equal_epsilon(inv.yz(), -0.330357f, EPSILON), "Matrix inversion failed on yz");
	REQUIRE_TRUE(float_equal_epsilon(inv.yw(), 0.107143f, EPSILON), "Matrix inversion failed on yw");

	REQUIRE_TRUE(float_equal_epsilon(inv.zx(), -0.669643f, EPSILON), "Matrix inversion failed on zx");
	REQUIRE_TRUE(float_equal_epsilon(inv.zy(), 0.660714f, EPSILON), "Matrix inversion failed on zy");
	REQUIRE_TRUE(float_equal_epsilon(inv.zz(), 0.991071f, EPSILON), "Matrix inversion failed on zz");
	REQUIRE_TRUE(float_equal_epsilon(inv.zw(), -0.821429f, EPSILON), "Matrix inversion failed on zw");

	REQUIRE_TRUE(float_equal_epsilon(inv.tx(), 0.535714f, EPSILON), "Matrix inversion failed on tx");
	REQUIRE_TRUE(float_equal_epsilon(inv.ty(), -0.428571f, EPSILON), "Matrix inversion failed on ty");
	REQUIRE_TRUE(float_equal_epsilon(inv.tz(), -0.392857f, EPSILON), "Matrix inversion failed on tz");
	REQUIRE_TRUE(float_equal_epsilon(inv.tw(), 0.357143f, EPSILON), "Matrix inversion failed on tw");
}

void MathTest::testQuaternions()
{
	Quaternion q1(3, 5, 7, 11);
	Quaternion q2(13, 17, 19, 23);
	const double EPSILON = 0.0001f;

	Quaternion product = q1.getMultipliedBy(q2);

	REQUIRE_TRUE(product.x() == 188, "Quaternion multiplication failed on x");
	REQUIRE_TRUE(product.y() == 336, "Quaternion multiplication failed on y");
	REQUIRE_TRUE(product.z() == 356, "Quaternion multiplication failed on z");
	REQUIRE_TRUE(product.w() == -4, "Quaternion multiplication failed on w");

	Quaternion q1multiplied = q1;
	q1multiplied.multiplyBy(q2);

	REQUIRE_TRUE(q1multiplied.x() == 188, "Quaternion in-place multiplication failed on x");
	REQUIRE_TRUE(q1multiplied.y() == 336, "Quaternion in-place multiplication failed on y");
	REQUIRE_TRUE(q1multiplied.z() == 356, "Quaternion in-place multiplication failed on z");
	REQUIRE_TRUE(q1multiplied.w() == -4, "Quaternion in-place multiplication failed on w");

	Quaternion q1inverted = q1.getInverse();

	REQUIRE_TRUE(q1inverted.x() == -3, "Quaternion inversion failed on x");
	REQUIRE_TRUE(q1inverted.y() == -5, "Quaternion inversion failed on y");
	REQUIRE_TRUE(q1inverted.z() == -7, "Quaternion inversion failed on z");
	REQUIRE_TRUE(q1inverted.w() == 11, "Quaternion inversion failed on w");

	Quaternion normalised = q1.getNormalised();

	REQUIRE_TRUE(float_equal_epsilon(normalised.x(), 0.01470588f, EPSILON), "Quaternion normalisation failed on x");
	REQUIRE_TRUE(float_equal_epsilon(normalised.y(), 0.02450980f, EPSILON), "Quaternion normalisation failed on y");
	REQUIRE_TRUE(float_equal_epsilon(normalised.z(), 0.03431372f, EPSILON), "Quaternion normalisation failed on z");
	REQUIRE_TRUE(float_equal_epsilon(normalised.w(), 0.05392156f, EPSILON), "Quaternion normalisation failed on w");

	Vector3 point(13, 17, 19);

	Vector3 transformed = q1.transformPoint(point);

	REQUIRE_TRUE(transformed.x() == q1.w()*q1.w()*point.x() + 2*q1.y()*q1.w()*point.z() - 2*q1.z()*q1.w()*point.y() + q1.x()*q1.x()*point.x() + 2*q1.y()*q1.x()*point.y() + 2*q1.z()*q1.x()*point.z() - q1.z()*q1.z()*point.x() - q1.y()*q1.y()*point.x(), "Quaternion point transformation failed on x");
	REQUIRE_TRUE(transformed.y() == 2*q1.x()*q1.y()*point.x() + q1.y()*q1.y()*point.y() + 2*q1.z()*q1.y()*point.z() + 2*q1.w()*q1.z()*point.x() - q1.z()*q1.z()*point.y() + q1.w()*q1.w()*point.y() - 2*q1.x()*q1.w()*point.z() - q1.x()*q1.x()*point.y(), "Quaternion point transformation failed on y");
	REQUIRE_TRUE(transformed.z() == 2*q1.x()*q1.z()*point.x() + 2*q1.y()*q1.z()*point.y() + q1.z()*q1.z()*point.z() - 2*q1.w()*q1.y()*point.x() - q1.y()*q1.y()*point.z() + 2*q1.w()*q1.x()*point.y() - q1.x()*q1.x()*point.z() + q1.w()*q1.w()*point.z(), "Quaternion point transformation failed on z");
}

// Initialise the static registrar object
Test::Registrar MathTest::_registrar(TestPtr(new MathTest));
