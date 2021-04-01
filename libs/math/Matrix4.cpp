#include "Matrix4.h"

#include "Quaternion.h"

#include <sstream>

namespace
{
	/// \brief Returns \p euler angles converted from degrees to radians.
	inline Vector3 euler_degrees_to_radians(const Vector3& euler)
	{
		return Vector3(
			degrees_to_radians(euler.x()),
			degrees_to_radians(euler.y()),
			degrees_to_radians(euler.z())
		);
	}

	inline bool quaternion_component_is_90(double component)
	{
		return (fabs(component) - c_half_sqrt2f) < 0.001f;
	}
}

// Main explicit constructor (private)
Matrix4::Matrix4(double xx_, double xy_, double xz_, double xw_,
						double yx_, double yy_, double yz_, double yw_,
						double zx_, double zy_, double zz_, double zw_,
						double tx_, double ty_, double tz_, double tw_)
{
    xx() = xx_;
    xy() = xy_;
    xz() = xz_;
    xw() = xw_;
    yx() = yx_;
    yy() = yy_;
    yz() = yz_;
    yw() = yw_;
    zx() = zx_;
    zy() = zy_;
    zz() = zz_;
    zw() = zw_;
    tx() = tx_;
    ty() = ty_;
    tz() = tz_;
    tw() = tw_;
}

// Named constructors

// Get a rotation from 2 vectors (named constructor)
Matrix4 Matrix4::getRotation(const Vector3& a, const Vector3& b)
{
	double angle = a.angle(b);
	Vector3 axis = b.crossProduct(a).getNormalised();

	return getRotation(axis, angle);
}

Matrix4 Matrix4::getRotation(const Vector3& axis, const double angle)
{
	// Pre-calculate the terms
	double cosPhi = cos(angle);
	double sinPhi = sin(angle);
	double oneMinusCosPhi = static_cast<double>(1) - cos(angle);
	double x = axis.x();
	double y = axis.y();
	double z = axis.z();
	return Matrix4::byColumns(
		cosPhi + oneMinusCosPhi*x*x, oneMinusCosPhi*x*y - sinPhi*z, oneMinusCosPhi*x*z + sinPhi*y, 0,
		oneMinusCosPhi*y*x + sinPhi*z, cosPhi + oneMinusCosPhi*y*y, oneMinusCosPhi*y*z - sinPhi*x, 0,
		oneMinusCosPhi*z*x - sinPhi*y, oneMinusCosPhi*z*y + sinPhi*x, cosPhi + oneMinusCosPhi*z*z, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotation(const Quaternion& quaternion)
{
	const double x2 = quaternion[0] + quaternion[0];
	const double y2 = quaternion[1] + quaternion[1];
	const double z2 = quaternion[2] + quaternion[2];
	const double xx = quaternion[0] * x2;
	const double xy = quaternion[0] * y2;
	const double xz = quaternion[0] * z2;
	const double yy = quaternion[1] * y2;
	const double yz = quaternion[1] * z2;
	const double zz = quaternion[2] * z2;
	const double wx = quaternion[3] * x2;
	const double wy = quaternion[3] * y2;
	const double wz = quaternion[3] * z2;

	return Matrix4::byColumns(
		1.0f - (yy + zz),
		xy + wz,
		xz - wy,
		0,
		xy - wz,
		1.0f - (xx + zz),
		yz + wx,
		0,
		xz + wy,
		yz - wx,
		1.0f - (xx + yy),
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationQuantised(const Quaternion& quaternion)
{
	if (quaternion.y() == 0 && quaternion.z() == 0 && quaternion_component_is_90(quaternion.x()) && quaternion_component_is_90(quaternion.w()))
	{
		return Matrix4::getRotationAboutXForSinCos((quaternion.x() > 0) ? 1.0f : -1.0f, 0);
	}

	if (quaternion.x() == 0 && quaternion.z() == 0 && quaternion_component_is_90(quaternion.y()) && quaternion_component_is_90(quaternion.w()))
	{
		return Matrix4::getRotationAboutYForSinCos((quaternion.y() > 0) ? 1.0f : -1.0f, 0);
	}

	if (quaternion.x() == 0	&& quaternion.y() == 0 && quaternion_component_is_90(quaternion.z()) && quaternion_component_is_90(quaternion.w()))
	{
		return Matrix4::getRotationAboutZForSinCos((quaternion.z() > 0) ? 1.0f : -1.0f, 0);
	}

	return getRotation(quaternion);
}

Matrix4 Matrix4::getRotationAboutXForSinCos(double s, double c)
{
	return Matrix4::byColumns(
		1, 0, 0, 0,
		0, c, s, 0,
		0,-s, c, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotationAboutYForSinCos(double s, double c)
{
	return Matrix4::byColumns(
		c, 0,-s, 0,
		0, 1, 0, 0,
		s, 0, c, 0,
		0, 0, 0, 1
	);
}

Matrix4 Matrix4::getRotationAboutZForSinCos(double s, double c)
{
	return Matrix4::byColumns(
		c, s, 0, 0,
		-s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}

/*! \verbatim
clockwise rotation around X, Y, Z, facing along axis
 1  0   0    cy 0 -sy   cz  sz 0
 0  cx  sx   0  1  0   -sz  cz 0
 0 -sx  cx   sy 0  cy   0   0  1

rows of Z by cols of Y
 cy*cz -sy*cz+sz -sy*sz+cz
-sz*cy -sz*sy+cz

  .. or something like that..

final rotation is Z * Y * X
 cy*cz -sx*-sy*cz+cx*sz  cx*-sy*sz+sx*cz
-cy*sz  sx*sy*sz+cx*cz  -cx*-sy*sz+sx*cz
 sy    -sx*cy            cx*cy

transposed
cy.cz + 0.sz + sy.0            cy.-sz + 0 .cz +  sy.0          cy.0  + 0 .0  +   sy.1       |
sx.sy.cz + cx.sz + -sx.cy.0    sx.sy.-sz + cx.cz + -sx.cy.0    sx.sy.0  + cx.0  + -sx.cy.1  |
-cx.sy.cz + sx.sz +  cx.cy.0   -cx.sy.-sz + sx.cz +  cx.cy.0   -cx.sy.0  + 0 .0  +  cx.cy.1  |
\endverbatim */
Matrix4 Matrix4::getRotationForEulerXYZ(const Vector3& euler)
{
	double cx = cos(euler[0]);
	double sx = sin(euler[0]);
	double cy = cos(euler[1]);
	double sy = sin(euler[1]);
	double cz = cos(euler[2]);
	double sz = sin(euler[2]);

	return Matrix4::byColumns(
		cy*cz,
		cy*sz,
		-sy,
		0,
		sx*sy*cz + cx*-sz,
		sx*sy*sz + cx*cz,
		sx*cy,
		0,
		cx*sy*cz + sx*sz,
		cx*sy*sz + -sx*cz,
		cx*cy,
		0,
		0,
		0,
		0,
		1
	);
}

Matrix4 Matrix4::getRotationForEulerXYZDegrees(const Vector3& euler)
{
	return getRotationForEulerXYZ(euler_degrees_to_radians(euler));
}

// Add a scale component
void Matrix4::scaleBy(const Vector3& scale)
{
    multiplyBy(getScale(scale));
}