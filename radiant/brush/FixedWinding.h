#ifndef FIXEDWINDING_H_
#define FIXEDWINDING_H_

#include "winding.h"

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
 *         with the standard size of MAX_POINTS_ON_WINDING.  
 */
class FixedWinding :
	public std::vector<FixedWindingVertex> 
{
public:
	FixedWinding() {
		reserve(MAX_POINTS_ON_WINDING);
	}
	
	// Writes the FixedWinding data into the given Winding
	inline void writeToWinding(Winding& winding) {
		winding.resize(size());
		winding.numpoints = size();
		
		for (std::size_t i = 0; i < size(); ++i) {
			winding[i].vertex[0] = (*this)[i].vertex[0];
			winding[i].vertex[1] = (*this)[i].vertex[1];
			winding[i].vertex[2] = (*this)[i].vertex[2];
			winding[i].adjacent = (*this)[i].adjacent;
		}
	}
};

#endif /*FIXEDWINDING_H_*/
