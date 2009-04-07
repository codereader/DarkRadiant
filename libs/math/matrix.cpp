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
    return Matrix4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translation.x(), translation.y(), translation.z(), 1
    );
}

// Get a rotation from 2 vectors (named constructor)
Matrix4 Matrix4::getRotation(const Vector3& a, const Vector3& b) 
{
	double angle = a.angle(b);
	Vector3 axis = b.crossProduct(a).getNormalised();
	// Pre-calculate the terms
	double cosPhi = cos(angle);
	double sinPhi = sin(angle);
	double oneMinusCosPhi = static_cast<double>(1) - cos(angle);
	double x = axis.x();
	double y = axis.y();
	double z = axis.z(); 
	return Matrix4(
		cosPhi + oneMinusCosPhi*x*x, oneMinusCosPhi*x*y - sinPhi*z, oneMinusCosPhi*x*z + sinPhi*y, 0,
		oneMinusCosPhi*y*x + sinPhi*z, cosPhi + oneMinusCosPhi*y*y, oneMinusCosPhi*y*z - sinPhi*x, 0,
		oneMinusCosPhi*z*x - sinPhi*y, oneMinusCosPhi*z*y + sinPhi*x, cosPhi + oneMinusCosPhi*z*z, 0,
		0, 0, 0, 1
	);
}

// Main explicit constructor
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

