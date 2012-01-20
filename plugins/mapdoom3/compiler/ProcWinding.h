#pragma once

#include "ibrush.h"
#include "math/Plane3.h"

namespace map
{

#define MAX_WORLD_COORD	( 128 * 1024 )
#define MIN_WORLD_COORD	( -128 * 1024 )
#define MAX_WORLD_SIZE	( MAX_WORLD_COORD - MIN_WORLD_COORD )

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

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

	void setFromPlane(const Plane3& plane)
	{
		IWinding::resize(4);

		Vector3 vup;

		float d = plane.normal().x() * plane.normal().x() + plane.normal().y() * plane.normal().y();

		if ( !d )
		{
			vup[0] = 1;
			vup[1] = 0;
			vup[2] = 0;
		}
		else
		{
			d = 1 / sqrt(d);
			vup[0] = -plane.normal().y() * d;
			vup[1] = plane.normal().x() * d;
			vup[2] = 0;
		}

		Vector3 vright = vup.crossProduct(plane.normal());
		
		vup *= MAX_WORLD_SIZE;
		vright *= MAX_WORLD_SIZE;

		Vector3 org = plane.normal() * plane.dist();

		(*this)[0].vertex = org - vright + vup;
		(*this)[0].texcoord = Vector2(0,0);

		(*this)[1].vertex = org + vright + vup;
		(*this)[1].texcoord = Vector2(0,0);

		(*this)[2].vertex = org + vright - vup;
		(*this)[2].texcoord = Vector2(0,0);

		(*this)[3].vertex = org - vright - vup;
		(*this)[3].texcoord = Vector2(0,0);
	}

	// Clips this winding against the given plane
	void clip(const Plane3& plane, const float epsilon = 0.0f)
	{
		std::size_t numPoints = IWinding::size();

		// Alloc space on the stack
		float* dists = (float*) _alloca((numPoints + 4) * sizeof(float));
		byte* sides = (byte*) _alloca((numPoints + 4) * sizeof(byte));

		int counts[3] = { 0,0,0 };

		// determine sides for each point
		std::size_t i;
		float dot;

		for (i = 0; i < numPoints; i++)
		{
			dists[i] = dot = plane.distanceToPointWinding((*this)[i].vertex);

			if (dot > epsilon)
			{
				sides[i] = SIDE_FRONT;
			}
			else if (dot < -epsilon)
			{
				sides[i] = SIDE_BACK;
			}
			else
			{
				sides[i] = SIDE_ON;
			}

			counts[sides[i]]++;
		}

		sides[i] = sides[0];
		dists[i] = dists[0];

		// if the winding is on the plane and we should keep it
		// FIXME: no keepOn yet
		//if ( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
//			return this;
		//}

		// if nothing at the front of the clipping plane
		if (!counts[SIDE_FRONT])
		{
			IWinding::clear();
			return;
		}

		// if nothing at the back of the clipping plane
		if (!counts[SIDE_BACK])
		{
			return;
		}

		std::size_t maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

		IWinding newPoints(maxpts);
		//newPoints = (idVec5 *) _alloca16( maxpts * sizeof( idVec5 ) );
		std::size_t newNumPoints = 0;
		
		for (i = 0; i < numPoints; i++)
		{
			const WindingVertex& p1 = (*this)[i];

			if (newNumPoints + 1 > maxpts)
			{
				return;		// can't split -- fall back to original
			}

			if (sides[i] == SIDE_ON)
			{
				newPoints[newNumPoints] = p1;
				newNumPoints++;
				continue;
			}

			if (sides[i] == SIDE_FRONT)
			{
				newPoints[newNumPoints] = p1;
				newNumPoints++;
			}

			if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			{
				continue;
			}

			if (newNumPoints + 1 > maxpts)
			{
				return;		// can't split -- fall back to original
			}

			// generate a split point
			const WindingVertex& p2 = (*this)[(i + 1) % numPoints];

			dot = dists[i] / (dists[i] - dists[i + 1]);

			WindingVertex mid;

			for (std::size_t j = 0; j < 3; j++)
			{
				// avoid round off error when possible
				if (plane.normal()[j] == 1.0f)
				{
					mid.vertex[j] = plane.dist();
				}
				else if (plane.normal()[j] == -1.0f)
				{
					mid.vertex[j] = -plane.dist();
				}
				else
				{
					mid.vertex[j] = p1.vertex[j] + dot * (p2.vertex[j] - p1.vertex[j]);
				}
			}

			mid.texcoord.x() = p1.texcoord[0] + dot * (p2.texcoord[0] - p1.texcoord[0]);
			mid.texcoord.y() = p1.texcoord[1] + dot * (p2.texcoord[1] - p1.texcoord[1]);

			newPoints[newNumPoints] = mid;
			newNumPoints++;
		}

		// Cut off any excess winding vertices
		newPoints.resize(newNumPoints);
	
		// Overwrite ourselves with the new winding
		swap(newPoints); 
	}
};

} // namespace
