#pragma once

#include "ProcFile.h"

namespace map
{

#define	XYZ_EPSILON	0.01
#define	ST_EPSILON	0.001
#define	COSINE_EPSILON	0.999

class OptUtils
{
public:
	// greebo: This is different to Matrix4::transform(Plane3), either this or the latter seems to make assumptions
	// of some sort.
	static Plane3 TransformPlane(const Plane3& plane, const Matrix4& m)
	{
		Plane3 transformed;

		// Ordinary vector transform
		transformed.normal().x() = m[0] * plane.normal().x() + m[4] * plane.normal().y() + m[8] * plane.normal().z();
		transformed.normal().y() = m[1] * plane.normal().x() + m[5] * plane.normal().y() + m[9] * plane.normal().z();
		transformed.normal().z() = m[2] * plane.normal().x() + m[6] * plane.normal().y() + m[10] * plane.normal().z();

		float offset = transformed.normal().x() * m[12] + transformed.normal().y() * m[13] + transformed.normal().z() * m[14];
		transformed.dist() = plane.dist() + offset;

		return transformed;
	}

	static bool MatchVert(const ArbitraryMeshVertex& a, const ArbitraryMeshVertex& b)
	{
		if (fabs(a.vertex[0] - b.vertex[0] ) > XYZ_EPSILON)
		{
			return false;
		}

		if (fabs(a.vertex[1] - b.vertex[1] ) > XYZ_EPSILON)
		{
			return false;
		}

		if (fabs(a.vertex[2] - b.vertex[2] ) > XYZ_EPSILON)
		{
			return false;
		}

		if (fabs(a.texcoord[0] - b.texcoord[0] ) > ST_EPSILON)
		{
			return false;
		}

		if (fabs(a.texcoord[1] - b.texcoord[1] ) > ST_EPSILON)
		{
			return false;
		}

		// if the normal is 0 (smoothed normals), consider it a match
		if (a.normal[0] == 0 && a.normal[1] == 0 && a.normal[2] == 0 && 
			b.normal[0] == 0 && b.normal[1] == 0 && b.normal[2] == 0)
		{
			return true;
		}

		// otherwise do a dot-product cosine check
		if (a.normal.dot(b.normal) < COSINE_EPSILON)
		{
			return false;
		}

		return true;
	}

	// an empty area will be considered invalid.
	// Due to some truly aweful epsilon issues, a triangle can switch between
	// valid and invalid depending on which order you look at the verts, so
	// consider it invalid if any one of the possibilities is invalid.
	static bool IsTriangleValid(const OptVertex* v1, const OptVertex* v2, const OptVertex* v3) 
	{
		Vector3 d1 = v2->pv - v1->pv;
		Vector3 d2 = v3->pv - v1->pv;

		Vector3 normal = d1.crossProduct(d2);

		if (normal[2] <= 0)
		{
			return false;
		}

		d1 = v3->pv - v2->pv;
		d2 = v1->pv - v2->pv;
		normal = d1.crossProduct(d2);

		if (normal[2] <= 0)
		{
			return false;
		}

		d1 = v1->pv - v3->pv;
		d2 = v2->pv - v3->pv;
		normal = d1.crossProduct(d2);

		if (normal[2] <= 0) 
		{
			return false;
		}

		return true;
	}

	// Returns false if it is either front or back facing
	static bool IsTriangleDegenerate(const OptVertex* v1, const OptVertex* v2, const OptVertex* v3)
	{
		Vector3 d1 = v2->pv - v1->pv;
		Vector3 d2 = v3->pv - v1->pv;
		Vector3 normal = d1.crossProduct(d2);

		return normal[2] == 0;
	}

	// Colinear is considdered crossing.
	static bool PointsStraddleLine(OptVertex* p1, OptVertex* p2, OptVertex* l1, OptVertex* l2)
	{
		bool t1 = IsTriangleDegenerate(l1, l2, p1);
		bool t2 = IsTriangleDegenerate(l1, l2, p2);

		if (t1 && t2) 
		{
			// colinear case
			float s1 = (p1->pv - l1->pv).dot(l2->pv - l1->pv);
			float s2 = (p2->pv - l1->pv).dot(l2->pv - l1->pv);
			float s3 = (p1->pv - l2->pv).dot(l2->pv - l1->pv);
			float s4 = (p2->pv - l2->pv).dot(l2->pv - l1->pv);

			bool positive = (s1 > 0 || s2 > 0 || s3 > 0 || s4 > 0);
			bool negative = (s1 < 0 || s2 < 0 || s3 < 0 || s4 < 0);

			return (positive && negative);
		} 
		else if (p1 != l1 && p1 != l2 && p2 != l1 && p2 != l2)
		{
			// no shared verts
			t1 = IsTriangleValid(l1, l2, p1);
			t2 = IsTriangleValid(l1, l2, p2);

			if (t1 && t2) 
			{
				return false;
			}

			t1 = IsTriangleValid(l1, p1, l2);
			t2 = IsTriangleValid(l1, p2, l2);

			if (t1 && t2)
			{
				return false;
			}

			return true;
		} 
		else 
		{
			// a shared vert, not colinear, so not crossing
			return false;
		}
	}


	static bool EdgesCross(OptVertex* a1, OptVertex* a2, OptVertex* b1, OptVertex* b2)
	{
		// if both verts match, consider it to be crossed
		if (a1 == b1 && a2 == b2)
		{
			return true;
		}

		if (a1 == b2 && a2 == b1)
		{
			return true;
		}

		// if only one vert matches, it might still be colinear, which
		// would be considered crossing

		// if both lines' verts are on opposite sides of the other
		// line, it is crossed
		if (!PointsStraddleLine(a1, a2, b1, b2))
		{
			return false;
		}

		if (!PointsStraddleLine(b1, b2, a1, a2))
		{
			return false;
		}

		return true;
	}
};

} // namespace
