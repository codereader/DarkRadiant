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
	
	/// \brief Keep the value of \p infinity as small as possible to improve precision in Winding_Clip.
	void createInfinite(const Plane3& plane, double infinity) {
		double max = -infinity;
		int x = -1;
		
		for (int i = 0; i < 3; i++) {
			double d = fabs(plane.normal()[i]);
			if (d > max) {
				x = i;
				max = d;
			}
		}
		
		if (x == -1) {
			globalErrorStream() << "invalid plane\n";
			return;
		}

		Vector3 vup = g_vector3_identity;
		switch (x) {
			case 0:
			case 1:
				vup[2] = 1;
				break;
			case 2:
				vup[0] = 1;
				break;
		}

		vup += plane.normal() * (-vup.dot(plane.normal()));
		vup.normalise();

		Vector3 org = plane.normal() * plane.dist();

		Vector3 vright = vup.crossProduct(plane.normal());

		vup *= infinity;
		vright *= infinity;

		// project a really big  axis aligned box onto the plane

		DoubleLine r1, r2, r3, r4;
		r1.origin = (org - vright) + vup;
		r1.direction = vright.getNormalised();
		push_back(FixedWindingVertex(r1.origin, r1, c_brush_maxFaces));
		
		r2.origin = org + vright + vup;
		r2.direction = (-vup).getNormalised();
		push_back(FixedWindingVertex(r2.origin, r2, c_brush_maxFaces));
		
		r3.origin = (org + vright) - vup;
		r3.direction = (-vright).getNormalised();
		push_back(FixedWindingVertex(r3.origin, r3, c_brush_maxFaces));
		
		r4.origin = (org - vright) - vup;
		r4.direction = vup.getNormalised();
		push_back(FixedWindingVertex(r4.origin, r4, c_brush_maxFaces));
	}
};

#endif /*FIXEDWINDING_H_*/
