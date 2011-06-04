#include "MathTest.h"

#include "math/matrix.h"

void MathTest::run()
{
	testIdentity();
	testMultiplication();
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
		REQUIRE_TRUE(transformed.y() == 9084, "Vector4 transformation failed");
		REQUIRE_TRUE(transformed.z() == 11214, "Vector4 transformation failed");
		REQUIRE_TRUE(transformed.w() == 12896, "Vector4 transformation failed");
	}
}

// Initialise the static registrar object
Test::Registrar MathTest::_registrar(TestPtr(new MathTest));
