#include "ProcWinding.h"

namespace map
{

void ProcWinding::setFromPlane(const Plane3& plane)
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
bool ProcWinding::clip(const Plane3& plane, const float epsilon)
{
	std::size_t numPoints = IWinding::size();

	// Alloc space on the stack
	float* dists = (float*)_alloca((numPoints + 4) * sizeof(float));
	unsigned char* sides = (unsigned char*)_alloca((numPoints + 4) * sizeof(unsigned char));

	int counts[3] = { 0,0,0 };

	// determine sides for each point
	std::size_t i;
	float dot;

	for (i = 0; i < numPoints; i++)
	{
		dists[i] = dot = plane.distanceToPoint((*this)[i].vertex);

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
		return false;
	}

	// if nothing at the back of the clipping plane
	if (!counts[SIDE_BACK])
	{
		return true;
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
			return true;		// can't split -- fall back to original
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
			return true;		// can't split -- fall back to original
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

	return true;
}

Vector3 ProcWinding::getCenter() const
{
	Vector3 center(0,0,0);

	for (std::size_t i = 0; i < IWinding::size(); ++i)
	{
		center += (*this)[i].vertex;
	}

	return center * (1.0f / IWinding::size());
}

Plane3 ProcWinding::getPlane() const
{
	if (IWinding::size() < 3 )
	{
		return Plane3(0,0,0,0);
	}

	Vector3 center = getCenter();

	Vector3 v1 = (*this)[0].vertex - center;
	Vector3 v2 = (*this)[1].vertex - center;

	Plane3 plane;

	plane.normal() = v2.crossProduct(v1);
	plane.normalise();

	// Fit the plane through the first point of this winding
	plane.dist() = plane.normal().dot((*this)[0].vertex);

	return plane;
}

int ProcWinding::split(const Plane3& plane, const float epsilon, ProcWinding& front, ProcWinding& back) const
{
	std::size_t numPoints = IWinding::size();

	float* dists = (float*)_alloca((numPoints+4) * sizeof(float));
	unsigned char* sides = (unsigned char*)_alloca((numPoints+4) * sizeof(unsigned char));

	int counts[3] = {0, 0, 0};

	// determine sides for each point
	std::size_t i;

	for (i = 0; i < numPoints; i++ )
	{
		float dot = dists[i] = plane.distanceToPoint((*this)[i].vertex);

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
	
	front.clear();
	back.clear();

	// if coplanar, put on the front side if the normals match
	if (!counts[SIDE_FRONT] && !counts[SIDE_BACK])
	{
		Plane3 windingPlane = getPlane();

		if (windingPlane.normal().dot(plane.normal()) > 0.0f)
		{
			front = *this;
			return SIDE_FRONT;
		}
		else
		{
			back = *this;
			return SIDE_BACK;
		}
	}

	// if nothing at the front of the clipping plane
	if (!counts[SIDE_FRONT])
	{
		back = *this;
		return SIDE_BACK;
	}

	// if nothing at the back of the clipping plane
	if (!counts[SIDE_BACK])
	{
		front = *this;
		return SIDE_FRONT;
	}

	std::size_t maxpts = numPoints + 4;	// cant use counts[0]+2 because of fp grouping errors

	front.reserve(maxpts);
	back.reserve(maxpts);
		
	for (i = 0; i < numPoints; ++i)
	{
		const IWinding::value_type& p1 = (*this)[i];
		
		if (sides[i] == SIDE_ON)
		{
			front.push_back(p1);
			back.push_back(p1);
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			front.push_back(p1);
		}

		if (sides[i] == SIDE_BACK)
		{
			back.push_back(p1);
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
		{
			continue;
		}
			
		// generate a split point
		const IWinding::value_type& p2 = (*this)[(i+1) % numPoints];
		IWinding::value_type mid;

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if (sides[i] == SIDE_FRONT)
		{
			float dot = dists[i] / (dists[i] - dists[i+1]);

			for (std::size_t j = 0; j < 3; ++j)
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

			mid.texcoord.x() = p1.texcoord.x() + dot * (p2.texcoord.x() - p1.texcoord.x());
			mid.texcoord.y() = p1.texcoord.y() + dot * (p2.texcoord.y() - p1.texcoord.y());
		}
		else
		{
			float dot = dists[i+1] / (dists[i+1] - dists[i]);

			for (std::size_t j = 0; j < 3; ++j)
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
					mid.vertex[j] = p2.vertex[j] + dot * (p1.vertex[j] - p2.vertex[j]);
				}
			}

			mid.texcoord.x() = p2.texcoord.x() + dot * (p1.texcoord.x() - p2.texcoord.x());
			mid.texcoord.y() = p2.texcoord.y() + dot * (p1.texcoord.y() - p2.texcoord.y());
		}

		front.push_back(mid);
		back.push_back(mid);
	}
	
	assert(front.size() <= maxpts && back.size() <= maxpts);

	return SIDE_CROSS;
}

int ProcWinding::planeSide(const Plane3& plane, const float epsilon) const
{
	bool front = false;
	bool back = false;

	for (std::size_t i = 0; i < size(); ++i)
	{
		float d = plane.distanceToPoint((*this)[i].vertex);

		if (d < -epsilon)
		{
			if (front)
			{
				return SIDE_CROSS;
			}

			back = true;
			continue;
		}
		else if (d > epsilon)
		{
			if (back)
			{
				return SIDE_CROSS;
			}

			front = true;
			continue;
		}
	}

	if (back)
	{
		return SIDE_BACK;
	}

	if (front)
	{
		return SIDE_FRONT;
	}

	return SIDE_ON;
}

bool ProcWinding::isTiny() const
{
	static const float EDGE_LENGTH_SQUARED = 0.2f * 0.2f;

	std::size_t edges = 0;
	std::size_t numPoints = IWinding::size();

	for (std::size_t i = 0; i < numPoints; ++i)
	{
		Vector3 delta = (*this)[(i + 1) % numPoints].vertex - (*this)[i].vertex;

		float len = delta.getLengthSquared();

		if (len > EDGE_LENGTH_SQUARED)
		{
			if (++edges == 3) // greebo: we need at least 3 planes that are above the threshold
			{
				return false;
			}
		}
	}

	return true;
}

bool ProcWinding::isHuge() const 
{
	std::size_t numPoints = IWinding::size();

	for (std::size_t i = 0; i < numPoints; ++i)
	{
		for (std::size_t j = 0; j < 3; ++j)
		{
			if ((*this)[i].vertex[j] <= MIN_WORLD_COORD || (*this)[i].vertex[j] >= MAX_WORLD_COORD ) 
			{
				return true;
			}
		}
	}

	return false;
}

float ProcWinding::getArea() const
{
	float total = 0.0f;
	std::size_t numPoints = IWinding::size();

	for (std::size_t i = 2; i < numPoints; ++i)
	{
		Vector3 d1 = (*this)[i-1].vertex - (*this)[0].vertex;
		Vector3 d2 = (*this)[i].vertex - (*this)[0].vertex;
		Vector3 cross = d1.crossProduct(d2);
		total += cross.getLength();
	}

	return total * 0.5f;
}

void ProcWinding::addToConvexHull(const ProcWinding& winding, const Vector3& normal, const float epsilon)
{
	if (winding.empty()) return;

	std::size_t maxPts = IWinding::size() + winding.size();

	IWinding::reserve(maxPts);

	//IWinding::value_type* newHullPoints = (IWinding::value_type*)_alloca(maxPts * sizeof(IWinding::value_type));
	Vector3* hullDirs = (Vector3*)_alloca(maxPts * sizeof(Vector3));
	bool* hullSide = (bool*)_alloca(maxPts * sizeof(bool));

	std::size_t j = 0;
	for (std::size_t i = 0; i < winding.size(); ++i)
	{
		const IWinding::value_type& p1 = winding[i];
		std::size_t numPoints = IWinding::size();

		// calculate hull edge vectors
		for (j = 0; j < numPoints; ++j)
		{
			Vector3 dir = (*this)[(j + 1) % numPoints].vertex - (*this)[j].vertex;
			dir.normalise();

			hullDirs[j] = normal.crossProduct(dir);
		}

		// calculate side for each hull edge
		bool outside = false;

		for (j = 0; j < numPoints; ++j)
		{
			Vector3 dir = p1.vertex - (*this)[j].vertex;

			float d = dir.dot(hullDirs[j]);

			if (d >= epsilon)
			{
				outside = true;
			}

			if (d >= -epsilon)
			{
				hullSide[j] = true;
			} 
			else 
			{
				hullSide[j] = false;
			}
		}

		// if the point is effectively inside, do nothing
		if (!outside) continue;

		// find the back side to front side transition
		for (j = 0; j < numPoints; ++j)
		{
			if (!hullSide[j] && hullSide[(j + 1) % numPoints])
			{
				break;
			}
		}

		if (j >= numPoints)
		{
			continue;
		}

		ProcWinding newHull;
		newHull.reserve(maxPts); // ensure size of target buffer

		// insert the point here
		newHull.push_back(p1);

		// copy over all points that aren't double fronts
		j = (j + 1) % numPoints;

		for (std::size_t k = 0; k < numPoints; ++k)
		{
			if (hullSide[(j + k) % numPoints] && hullSide[(j + k + 1) % numPoints])
			{
				continue;
			}

			newHull.push_back((*this)[(j + k + 1) % numPoints]);
		}

		this->swap(newHull);
	}
}

float ProcWinding::getTriangleArea(const Vector3& a, const Vector3& b, const Vector3& c)
{
	Vector3 v1 = b - a;
	Vector3 v2 = c - a;

	return 0.5f * v1.crossProduct(v2).getLength();
}

} // namespace
