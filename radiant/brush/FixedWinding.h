#ifndef FIXEDWINDING_H_
#define FIXEDWINDING_H_

#include "math/Vector3.h"
#include "math/Plane3.h"

class Winding;

#define MAX_POINTS_ON_WINDING 64

class DoubleLine {
public:
	Vector3 origin;
	Vector3 direction;
	
	/// \brief Returns the point at which \p line intersects \p plane, 
	// or an undefined value if there is no intersection.
	inline Vector3 intersectPlane(const Plane3& plane) const {
		return origin + direction * (-plane.distanceToPoint(origin) / direction.dot(plane.normal()));
	}
};

class FixedWindingVertex {
public:
	Vector3 vertex;
	DoubleLine edge;
	std::size_t adjacent;

	FixedWindingVertex(const Vector3& vertex_, const DoubleLine& edge_,	std::size_t adjacent_) :
		vertex(vertex_), 
		edge(edge_), 
		adjacent(adjacent_)
	{}
};

/**
 * greebo: A FixedWinding is a vector of FixedWindingVertices
 *         with a pre-allocated size of MAX_POINTS_ON_WINDING.  
 */
class FixedWinding :
	public std::vector<FixedWindingVertex> 
{
public:
	FixedWinding() {
		reserve(MAX_POINTS_ON_WINDING);
	}
	
	// Writes the FixedWinding data into the given Winding
	void writeToWinding(Winding& winding);
	
	/// \brief Keep the value of \p infinity as small as possible to improve precision in Winding_Clip.
	void createInfinite(const Plane3& plane, double infinity);
	
	/// \brief Clip this winding which lies on \p plane by \p clipPlane, resulting in \p clipped.
	/// If \p winding is completely in front of the plane, \p clipped will be identical to \p winding.  
	/// If \p winding is completely in back of the plane, \p clipped will be empty.  
	/// If \p winding intersects the plane, the edge of \p clipped which lies on \p clipPlane will store the value of \p adjacent.
	void clip(const Plane3& plane, const Plane3& clipPlane, std::size_t adjacent, FixedWinding& clipped);
};

#endif /*FIXEDWINDING_H_*/
