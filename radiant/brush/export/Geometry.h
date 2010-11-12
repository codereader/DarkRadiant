#ifndef CM_GEOMETRY_H_
#define CM_GEOMETRY_H_

#include <map>
#include <vector>
#include "math/Vector3.h"
#include "math/Plane3.h"

/** greebo: This contains all the geometry subtypes of a Doom3 CollisionModel.
 **/

namespace cmutil {

// A list of indexed ("named") vertices
typedef std::vector<std::size_t> VertexList;

// The indexed vertices of the collisionmodel
typedef std::map<std::size_t, Vector3> VertexMap;

struct Edge {
	std::size_t from;	// The starting vertex index
	std::size_t to;	// The end vertex index
	std::size_t numVertices; // At least I think it's numVertices

	Edge(std::size_t num = 2) :
		from(0),
		to(0),
		numVertices(num)
	{}
};

// The indexed Edges (each consisting of a start/end vertex)
typedef std::map<std::size_t, Edge> EdgeMap;
// A vector of Edges defining a polygon (the sign indicates the direction)
typedef std::vector<int> EdgeList;

struct Polygon {
	// The number of edges of this polygon
	std::size_t numEdges;

	// The indices of the edges forming this polygon
	EdgeList edges;

	// The plane (normal + distance)
	Plane3 plane;

	// Two vertices defining the AABB
	Vector3 min;
	Vector3 max;

	// The shadername (this is textures/common/collision for
	// proper Doom3 hulls, but it can be different as well).
	std::string shader;
};

// The unsorted list of Polygons
typedef std::vector<Polygon> PolygonList;

typedef std::vector<Plane3> PlaneList;

struct BrushStruc {
	std::size_t numFaces;
	PlaneList planes;
	// Two points defining the AABB
	Vector3 min;
	Vector3 max;
};

typedef std::vector<BrushStruc> BrushList;

} // namespace cmutil

#endif /*CM_GEOMETRY_H_*/
