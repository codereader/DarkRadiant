#ifndef COLLISIONMODEL_H_
#define COLLISIONMODEL_H_

#include <map>
#include "math/Vector3.h"
#include "math/Plane3.h"
#include "selection/algorithm/Primitives.h"

struct Winding;

namespace selection {

	namespace {
		const std::string RKEY_COLLISION_SHADER = "game/defaults/collisionTexture";
	}

class CollisionModel
{
	// The indexed vertices of the collisionmodel
	typedef std::map<unsigned int, Vector3> VertexMap;
	
	struct Edge {
		unsigned int from;	// The starting vertex index
		unsigned int to;	// The end vertex index
	};
	
	// The indexed Edges (each consisting of a start/end vertex)
	typedef std::map<unsigned int, Edge> EdgeMap;
	
	struct Polygon {
		// The number of edges of this polygon
		unsigned int numEdges;
		
		// The indices of the edges forming this polygon
		std::vector<unsigned int> edges;
		
		// The plane (normal + distance)
		Plane3 plane;
		
		// Two vertices defining the AABB
		Vector3 min;
		Vector3 max;
	};
	
	// The unsorted list of Polygons
	typedef std::vector<Polygon> PolygonList;
	
	typedef std::vector<Plane3> PlaneList;
	
	struct BrushStruc {
		unsigned int numFaces;
		PlaneList planes;
		// Two points defining the AABB
		Vector3 min;
		Vector3 max;
	};
	
	typedef std::vector<BrushStruc> BrushList;
	
	// The container instances with all the vertices/edges/faces
	VertexMap _vertices;
	EdgeMap _edges;
	PolygonList _polygons;
	BrushList _brushes;

public:
	void addBrush(Brush& brush);
	
	bool isValid() const;

	/** greebo: Stream insertion operator, use this to write
	 * the collision model into a file. Qualified as "friend" to allow the access 
	 * of private members and the first function argument to be std::ostream.
	 */
	friend std::ostream& operator<<(std::ostream& st, const CollisionModel& cm);

	/** greebo: The command target
	 */
	static void createFromSelection();

private:
	/** greebo: Adds the given vertex to the internal vertex list
	 * and returns its index. If the vertex already exists,
	 * the index to the existing vertex is returned.
	 * 
	 * @returns: the index of the (existing/inserted) vertex.
	 */
	unsigned int addVertex(const Vector3& vertex);
	
	/** greebo: Tries to lookup the index of the given vertex.
	 * 
	 * @returns: the index of the vertex or -1 if not found
	 */
	int findVertex(const Vector3& vertex) const;
	
	/** greebo: "Parses" the given Winding and adds its
	 * 			geometry info (vertices, edges, polys) into the maps.
	 */
	void addWinding(const Winding& winding);
	
	/** greebo: Adds the given edge to the internal edge map
	 * and returns its index. If the edge already exists,
	 * the index to the existing edge is returned.
	 * 
	 * @returns: the index of the (existing/inserted) edge.
	 */
	unsigned int addEdge(const Edge& edge);
	
	/** greebo: Tries to lookup the index of the given edge, 
	 * 			regardless of the direction / order of the
	 * 			contained vertex indices
	 * 
	 * @returns: the index of the edge or -1 if not found
	 */
	int findEdge(const Edge& edge) const;
};

typedef boost::shared_ptr<CollisionModel> CollisionModelPtr;

} // namespace selection

#endif /*COLLISIONMODEL_H_*/
