#pragma once

#include "Matrix4.h"

/**
 * Specialisation of the Matrix4 class offering convenience methods
 * for viewprojection-related calculations.
 */
class ViewProjection :
	public Matrix4
{
public:
	ViewProjection() :
		Matrix4()
	{}

	ViewProjection(const Matrix4& other) :
		Matrix4(other)
	{}

	bool testPoint(const Vector3& point) const;

	bool testPoint(const Vector3& point, const Matrix4& localToWorld) const;
};

inline bool ViewProjection::testPoint(const Vector3& point) const
{
	Vector4 hpoint = transform(Vector4(point, 1.0f));

	if (fabs(hpoint[0]) < fabs(hpoint[3]) && 
		fabs(hpoint[1]) < fabs(hpoint[3]) && 
		fabs(hpoint[2]) < fabs(hpoint[3]))
	{
		return true;
	}

	return false;
}

inline bool ViewProjection::testPoint(const Vector3& point, const Matrix4& localToWorld) const
{
	return testPoint(localToWorld.transformPoint(point));
}
