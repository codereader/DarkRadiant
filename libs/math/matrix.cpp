/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "matrix.h"

// Named constructors

// Identity matrix
const Matrix4& Matrix4::getIdentity()
{
    static const Matrix4 _identity(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    return _identity;
}

// Get a translation matrix for the given vector
Matrix4 Matrix4::getTranslation(const Vector3& translation)
{
    return Matrix4::byColumns(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x(), translation.y(), translation.z(), 1
    );
}

// Get a rotation from 2 vectors (named constructor)
Matrix4 Matrix4::getRotation(const Vector3& a, const Vector3& b)
{
	float angle = a.angle(b);
	Vector3 axis = b.crossProduct(a).getNormalised();

	return getRotation(axis, angle);
}

Matrix4 Matrix4::getRotation(const Vector3& axis, const float angle)
{
	// Pre-calculate the terms
	float cosPhi = cos(angle);
	float sinPhi = sin(angle);
	float oneMinusCosPhi = static_cast<float>(1) - cos(angle);
	float x = axis.x();
	float y = axis.y();
	float z = axis.z();
	return Matrix4::byColumns(
		cosPhi + oneMinusCosPhi*x*x, oneMinusCosPhi*x*y - sinPhi*z, oneMinusCosPhi*x*z + sinPhi*y, 0,
		oneMinusCosPhi*y*x + sinPhi*z, cosPhi + oneMinusCosPhi*y*y, oneMinusCosPhi*y*z - sinPhi*x, 0,
		oneMinusCosPhi*z*x - sinPhi*y, oneMinusCosPhi*z*y + sinPhi*x, cosPhi + oneMinusCosPhi*z*z, 0,
		0, 0, 0, 1
	);
}

// Get a scale matrix
Matrix4 Matrix4::getScale(const Vector3& scale)
{
    return Matrix4::byColumns(
        scale[0], 0, 0, 0,
        0, scale[1], 0, 0,
        0, 0, scale[2], 0,
        0, 0, 0,        1
    );
}

// Transpose the matrix in-place
void Matrix4::transpose()
{
    std::swap(_m[1], _m[4]); // xy <=> yx
    std::swap(_m[2], _m[8]); // xz <=> zx
    std::swap(_m[3], _m[12]); // xw <=> tx
    std::swap(_m[6], _m[9]); // yz <=> zy
    std::swap(_m[7], _m[13]); // yw <=> ty
    std::swap(_m[11], _m[14]); // zw <=> tz
}

// Return transposed copy
Matrix4 Matrix4::getTransposed() const
{
    return Matrix4(
        xx(), yx(), zx(), tx(),
        xy(), yy(), zy(), ty(),
        xz(), yz(), zz(), tz(),
        xw(), yw(), zw(), tw()
    );
}

// Return affine inverse
Matrix4 Matrix4::getInverse() const
{
  Matrix4 result;

  // determinant of rotation submatrix
  float det
    = _m[0] * ( _m[5]*_m[10] - _m[9]*_m[6] )
    - _m[1] * ( _m[4]*_m[10] - _m[8]*_m[6] )
    + _m[2] * ( _m[4]*_m[9] - _m[8]*_m[5] );

  // throw exception here if (det*det < 1e-25)

  // invert rotation submatrix
  det = 1.0f / det;

  result[0] = (  (_m[5]*_m[10]- _m[6]*_m[9] )*det);
  result[1] = (- (_m[1]*_m[10]- _m[2]*_m[9] )*det);
  result[2] = (  (_m[1]*_m[6] - _m[2]*_m[5] )*det);
  result[3] = 0;
  result[4] = (- (_m[4]*_m[10]- _m[6]*_m[8] )*det);
  result[5] = (  (_m[0]*_m[10]- _m[2]*_m[8] )*det);
  result[6] = (- (_m[0]*_m[6] - _m[2]*_m[4] )*det);
  result[7] = 0;
  result[8] = (  (_m[4]*_m[9] - _m[5]*_m[8] )*det);
  result[9] = (- (_m[0]*_m[9] - _m[1]*_m[8] )*det);
  result[10]= (  (_m[0]*_m[5] - _m[1]*_m[4] )*det);
  result[11] = 0;

  // multiply translation part by rotation
  result[12] = - (_m[12] * result[0] +
    _m[13] * result[4] +
    _m[14] * result[8]);
  result[13] = - (_m[12] * result[1] +
    _m[13] * result[5] +
    _m[14] * result[9]);
  result[14] = - (_m[12] * result[2] +
    _m[13] * result[6] +
    _m[14] * result[10]);
  result[15] = 1;

  return result;
}

// Transform a vector
Vector4 Matrix4::transform(const Vector4& vector4) const
{
    return Vector4(
        _m[0] * vector4[0] + _m[4] * vector4[1] + _m[8]  * vector4[2] + _m[12] * vector4[3],
        _m[1] * vector4[0] + _m[5] * vector4[1] + _m[9]  * vector4[2] + _m[13] * vector4[3],
        _m[2] * vector4[0] + _m[6] * vector4[1] + _m[10] * vector4[2] + _m[14] * vector4[3],
        _m[3] * vector4[0] + _m[7] * vector4[1] + _m[11] * vector4[2] + _m[15] * vector4[3]
    );
}

// Transform a plane
Plane3 Matrix4::transform(const Plane3& plane) const
{
    Plane3 transformed;
    transformed.normal().x() = _m[0] * plane.normal().x() + _m[4] * plane.normal().y() + _m[8] * plane.normal().z();
    transformed.normal().y() = _m[1] * plane.normal().x() + _m[5] * plane.normal().y() + _m[9] * plane.normal().z();
    transformed.normal().z() = _m[2] * plane.normal().x() + _m[6] * plane.normal().y() + _m[10] * plane.normal().z();
    transformed.dist() = -(	(-plane.dist() * transformed.normal().x() + _m[12]) * transformed.normal().x() +
                        (-plane.dist() * transformed.normal().y() + _m[13]) * transformed.normal().y() +
                        (-plane.dist() * transformed.normal().z() + _m[14]) * transformed.normal().z());
    return transformed;
}

// Inverse transform a plane
Plane3 Matrix4::inverseTransform(const Plane3& plane) const
{
    return Plane3(
        _m[ 0] * plane.normal().x() + _m[ 1] * plane.normal().y() + _m[ 2] * plane.normal().z() + _m[ 3] * plane.dist(),
        _m[ 4] * plane.normal().x() + _m[ 5] * plane.normal().y() + _m[ 6] * plane.normal().z() + _m[ 7] * plane.dist(),
        _m[ 8] * plane.normal().x() + _m[ 9] * plane.normal().y() + _m[10] * plane.normal().z() + _m[11] * plane.dist(),
        _m[12] * plane.normal().x() + _m[13] * plane.normal().y() + _m[14] * plane.normal().z() + _m[15] * plane.dist()
    );
}

// Multiply by another matrix, in-place
void Matrix4::multiplyBy(const Matrix4& other)
{
    *this = getMultipliedBy(other);
}

// Add a translation component
void Matrix4::translateBy(const Vector3& translation)
{
    multiplyBy(getTranslation(translation));
}

// Add a scale component
void Matrix4::scaleBy(const Vector3& scale)
{
    multiplyBy(getScale(scale));
}

