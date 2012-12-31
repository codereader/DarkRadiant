#pragma once

#include "Vector4.h"
#include "Plane3.h"
#include "Matrix4.h"
#include "ViewProjection.h"

class Viewer : 
	public Vector4
{
public:
	Viewer() :
		Vector4()
	{}

	Viewer(const Vector4& other) :
		Vector4(other)
	{}

	Viewer(const Vector3& vec3, float w) :
		Vector4(vec3, w)
	{}

	static Viewer createFromTransformedViewer(const Vector4& viewer, const Matrix4& transform);

	static Viewer createFromViewProjection(const ViewProjection& viewproj);

	bool testPlane(const Plane3& plane) const;

	bool testPlane(const Plane3& plane, const Matrix4& localToWorld) const;

	bool testTriangle(const Vector3& p0, const Vector3& p1, const Vector3& p2) const;
};

inline bool Viewer::testPlane(const Plane3& plane) const
{
	return (plane.normal().x() * x()) + (plane.normal().y() * y()) + 
		   (plane.normal().z() * z()) + (plane.dist() * w()) > 0;
}

inline bool Viewer::testPlane(const Plane3& plane, const Matrix4& localToWorld) const
{
	return testPlane(plane.transformed(localToWorld));
}

inline Viewer Viewer::createFromTransformedViewer(const Vector4& viewer, const Matrix4& transform)
{
	if (viewer.w() == 0)
	{
		return Viewer(transform.transformDirection(viewer.getVector3()), 0);
	}
	else
	{
		return Viewer(transform.transformPoint(viewer.getVector3()), viewer.w());
	}
}

inline Vector3 triangle_cross(const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	return (p1 - p0).crossProduct(p1 - p2);
}

inline bool Viewer::testTriangle(const Vector3& p0, const Vector3& p1, const Vector3& p2) const
{
	Vector3 cross = triangle_cross(p0, p1, p2);

	return (x() * cross[0]) + (y() * cross[1]) + (z() * cross[2]) > 0;
}

inline Viewer Viewer::createFromViewProjection(const ViewProjection& viewproj)
{
	// get viewer pos in object coords
	Vector4 viewer(viewproj.getFullInverse().transform(Vector4(0, 0, -1, 0)));

	if (viewer[3] != 0) // non-affine matrix
	{
		viewer[0] /= viewer[3];
		viewer[1] /= viewer[3];
		viewer[2] /= viewer[3];
		viewer[3] /= viewer[3];
	}
	
	return viewer;
}
