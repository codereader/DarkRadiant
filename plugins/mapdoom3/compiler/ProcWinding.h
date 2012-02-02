#pragma once

#include "ibrush.h"
#include "math/Plane3.h"

namespace map
{

// TODO: hack
#ifdef __linux__
#define _alloca alloca
#endif

#define MAX_WORLD_COORD	( 128 * 1024 )
#define MIN_WORLD_COORD	( -128 * 1024 )
#define MAX_WORLD_SIZE	( MAX_WORLD_COORD - MIN_WORLD_COORD )

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define	SIDE_CROSS	3

#define ON_EPSILON 0.1f

class ProcWinding : 
	public IWinding
{
public:
	ProcWinding() :
		IWinding()
	{}

	ProcWinding(const Plane3& plane) :
		IWinding(4) // 4 points for this plane
	{
		setFromPlane(plane);
	}

	ProcWinding(const Vector3& a, const Vector3& b, const Vector3& c) :
		IWinding(3)
	{
		(*this)[0].vertex = a;
		(*this)[1].vertex = b;
		(*this)[2].vertex = c;
	}

	// Creates a near-infinitely large winding from the given plane
	void setFromPlane(const Plane3& plane);

	// Clips this winding against the given plane,
	// returns true if some part was at the front
	bool clip(const Plane3& plane, const float epsilon = 0.0f);

	// Splits this winding in two, using the given plane as splitter. This winding itself stays unaltered
	int split(const Plane3& plane, const float epsilon, ProcWinding& front, ProcWinding& back) const;

	int planeSide(const Plane3& plane, const float epsilon = ON_EPSILON) const;

	// A winding is not tiny if at least three edges are above a certain threshold
	bool isTiny() const;

	// A base winding made from a plane is typically huge
	bool isHuge() const;

	float getArea() const;

	Vector3 getCenter() const;

	// Adds the given winding to the convex hull.
	// Assumes the current winding already is a convex hull with three or more points.
	void addToConvexHull(const ProcWinding& winding, const Vector3& normal, const float epsilon = ON_EPSILON);

	static float getTriangleArea(const Vector3& a, const Vector3& b, const Vector3& c);

private:
	Plane3 getPlane() const;
};

} // namespace

