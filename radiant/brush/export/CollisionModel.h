#ifndef COLLISIONMODEL_H_
#define COLLISIONMODEL_H_

#include "Geometry.h"

class Winding;
class Brush;
class Face;

namespace cmutil {

	namespace {
		const std::string RKEY_COLLISION_SHADER = "game/defaults/collisionTexture";
		const std::string ECLASS_CLIPMODEL = "func_clipmodel";
	}

class CollisionModel
{
	// The container instances with all the vertices/edges/faces
	VertexMap _vertices;
	EdgeMap _edges;
	PolygonList _polygons;
	BrushList _brushes;
	
	std::string _model;

public:
	CollisionModel();

	void addBrush(Brush& brush);
	
	/** greebo: Stream insertion operator, use this to write
	 * the collision model into a file. Qualified as "friend" to allow the access 
	 * of private members and the first function argument to be std::ostream.
	 */
	friend std::ostream& operator<<(std::ostream& st, const CollisionModel& cm);
	
	/** greebo: Sets the model path this CM is associated with 
	 */
	void setModel(const std::string& model);

	/** greebo: Calculates the "BrushMemory" number by using the constants
	 * 			defined in the .cpp file. 
	 */
	static unsigned int getBrushMemory(const BrushList& brushes);

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
	 * 
	 * @returns: the VertexList defining the Winding points in a 
	 * 			 closed loop (last vertexId = first vertexId)
	 */
	VertexList addWinding(const Winding& winding);
	
	/** greebo: Adds the given edge to the internal edge map
	 * and returns its index. If the edge already exists,
	 * the index to the existing edge is returned.
	 * 
	 * @returns: the index of the (existing/inserted) edge.
	 */
	unsigned int addEdge(const Edge& edge);
	
	/** greebo: Tries to lookup the index of the given edge, 
	 * 			and returns the index with the factor +1/-1
	 * 			according to the direction. 
	 * 
	 * @returns: +index / -index of the edge or 0 for the NULL edge
	 */
	int findEdge(const Edge& edge) const;
	
	/** greebo: Tries to lookup the index of the matching polygon. 
	 * 			All the Edge indices are compared regardless of 
	 * 			their order. 
	 * 
	 * @returns: the index of the polygon or -1 if not found
	 */
	int findPolygon(const EdgeList& otherEdges);
	
	/** greebo: Adds a polygon basing on the given face & vertexlist.
	 * 			Be sure to add the first vertex a second time
	 * 			to the end of the pass a "closed" winding.
	 * 			Duplicate polygons are not added.
	 */
	void addPolygon(const Face& face, const VertexList& vertexList);
};

typedef boost::shared_ptr<CollisionModel> CollisionModelPtr;

} // namespace cmutil

#endif /*COLLISIONMODEL_H_*/
