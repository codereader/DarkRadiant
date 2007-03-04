#ifndef COLLISIONMODEL_H_
#define COLLISIONMODEL_H_

#include <map>
#include "math/Vector3.h"
#include "selection/algorithm/Primitives.h"

namespace selection {

	namespace {
		const std::string RKEY_COLLISION_SHADER = "game/defaults/collisionTexture";
	}

class CollisionModel
{
	// The indexed vertices of the collisionmodel
	typedef std::map<unsigned int, Vector3> Vertices;
	
	struct Edge {
		unsigned int from;	// The starting vertex index
		unsigned int to;	// The end vertex index
	};
	
	// The indexed Edges (each consisting of a start/end vertex)
	typedef std::map<unsigned int, Edge> Edges;
	
	struct Polygon {
		// The number of edges of this polygon
		unsigned int numEdges;
		
		// The indices of the edges forming this polygon
		std::vector<unsigned int> edges;
		
		// The normal vector of this polygon
		Vector3 normal;
		
		// Two (opposite?) points of this polygon
		Vector3 point1;
		Vector3 point2;
	};
	
	// The unsorted list of Polygons
	typedef std::vector<Polygon> Polygons;
	
	BrushPtrVector _brushes;
	
public:
	void addBrush(Brush& brush);
	
	bool isValid() const;

	static void createFromSelection();
};

typedef boost::shared_ptr<CollisionModel> CollisionModelPtr;

} // namespace selection

#endif /*COLLISIONMODEL_H_*/
