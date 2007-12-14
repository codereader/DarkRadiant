#ifndef FIXEDWINDING_H_
#define FIXEDWINDING_H_

#include "math/Vector3.h"

class Winding;
class Plane3;

#define MAX_POINTS_ON_WINDING 64

struct DoubleLine {
	Vector3 origin;
	Vector3 direction;
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
};

#endif /*FIXEDWINDING_H_*/
