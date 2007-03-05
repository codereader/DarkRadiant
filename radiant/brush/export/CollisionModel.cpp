#include "CollisionModel.h"

#include "stream/textstream.h"
#include "itextstream.h"
#include "iselection.h"
#include "ientity.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "brush/Brush.h"
#include "winding.h"

namespace cmutil {

// Writes the given Vector3 in the format ( 0 1 2 ) to the given stream
void writeVector(std::ostream& st, const Vector3& vector) {
	st << "( ";
	st << vector[0] << " ";
	st << vector[1] << " ";
	st << vector[2] << " ";
	st << ")";
}

// Writes a single polygon to the given stream <st>
std::ostream& operator<< (std::ostream& st, const Polygon poly) {
	st << poly.numEdges;
	
	// Edge List
	st << " (";
	for (unsigned int e = 0; e < poly.edges.size(); e++) {
		st << " " << poly.edges[e];
	}
	st << " ) ";
		
	// Plane normal
	writeVector(st, poly.plane.normal());
	st << " " << poly.plane.dist() << " ";
	writeVector(st, poly.min);
	st << " ";
	writeVector(st, poly.max);
	st << " \"" << poly.shader.c_str() << "\"";
	return st;
}

std::ostream& operator<< (std::ostream& st, const BrushStruc b) {
	st << b.numFaces << " {\n";
		
	// Write all the planes
	for (unsigned int i = 0; i < b.planes.size(); i++) {
		st << "\t\t";
		writeVector(st, b.planes[i].normal());
		st << " " << b.planes[i].dist() << "\n";
	}
	
	st << "\t} ";
	
	// Write the two AABB vertices
	writeVector(st, b.min);
	st << " ";
	writeVector(st, b.max);
	st << " ";
	
	// Now append the "solid"
	st << "\"solid\"";
	return st;
}

CollisionModel::CollisionModel() {
	// Create the "NULL" edge (numVertices = 0)
	_edges[0] = Edge(0);
}

int CollisionModel::findVertex(const Vector3& vertex) const {
	for (VertexMap::const_iterator i = _vertices.begin(); 
		i != _vertices.end(); 
		i++) 
	{
		if (i->second == vertex) {
			return i->first;
		}
	}
	return -1;
}

unsigned int CollisionModel::addVertex(const Vector3& vertex) {
	// Try to lookup the index of the given vertex
	int foundIndex = findVertex(vertex);
	
	if (foundIndex == -1) {
		// Insert the vertex at the end of the VertexMap
		// The size of the map is the highest index + 1
		unsigned int lastIndex = _vertices.size();
		_vertices[lastIndex] = vertex;
		
		return lastIndex;
	}
	else {
		// Return the found index
		return static_cast<unsigned int>(foundIndex);
	} 
}

int CollisionModel::findEdge(const Edge& edge) const {
	for (EdgeMap::const_iterator i = _edges.begin(); 
		 i != _edges.end(); 
		 i++) 
	{
		// Direction match?
		if (i->second.from == edge.from && i->second.to == edge.to) {
			return i->first;
		}
		
		// Opposite direction match?
		if (i->second.from == edge.to && i->second.to == edge.from) {
			return -i->first;
		}
	}
	return 0;
}  

unsigned int CollisionModel::addEdge(const Edge& edge) {
	// Check for existing edge!
	int foundIndex = findEdge(edge);
	
	if (foundIndex == 0) {
		// NULL edge found, insert the edge with a new index
		unsigned int edgeIndex = _edges.size();
		_edges[edgeIndex] = edge;
		return edgeIndex;
	}
	else {
		return abs(foundIndex);
	}
}

Polygon CollisionModel::addPolygon(
	const Face& face, 
	const VertexList& vertexList) 
{
	Polygon poly;
	
	// Cycle from the beginning to the end-1 and add the edges 
	for (unsigned int i = 0; i < vertexList.size()-1; i++) {
		Edge edge;
		edge.from = vertexList[i];
		edge.to = vertexList[i+1];
		
		// Lookup the edge (the sign is interpreted as direction) 
		// and add it to the edge list
		poly.edges.push_back(findEdge(edge));
	}
	
	AABB faceAABB = face.getWinding().aabb();
		
	poly.numEdges = poly.edges.size();
	poly.plane = face.plane3();
	poly.min = faceAABB.origin - faceAABB.extents;
	poly.max = faceAABB.origin + faceAABB.extents;
	poly.shader = face.GetShader();
	
	return poly;
}

VertexList CollisionModel::addWinding(
	const Winding& winding)
{
	VertexList vertexList;
	
	for (Winding::const_iterator i = winding.begin(); i != winding.end(); i++) {
		// Create a vertexId and add it to the stack
		vertexList.push_back(addVertex(i->vertex));
	}
	// Now add the first vertex a second time to the end of the list
	vertexList.push_back(addVertex(winding.begin()->vertex));
	
	if (vertexList.size() > 1) {
		Edge edge;
		
		// Now work through the stack, adding the edges (note the -1 in the for condition)
		for (unsigned int i = 0; i < vertexList.size()-1; i++) {
			edge.from = vertexList[i];
			edge.to = vertexList[i+1];
			
			addEdge(edge);
		}
	}
	else {
		globalErrorStream() << "Warning: degenerate winding found.\n";
	}
	
	// Now that all edges are added, return the VertexList defining the winding
	return vertexList;
}

void CollisionModel::addBrush(Brush& brush) {
	BrushStruc b;

	// The number of faces
	b.numFaces = brush.size();
	
	// Get the AABB of this brush
	AABB brushAABB = brush.localAABB();
	
	b.min = brushAABB.origin - brushAABB.extents;
	b.max = brushAABB.origin + brushAABB.extents;
	
	// Populate the FaceList
	for (Brush::const_iterator i = brush.begin(); i != brush.end(); i++) {
		// Store the plane into the brush
		b.planes.push_back((*i)->plane3());
		
		// Parse the winding of this Face for vertices/edges
		VertexList vertexList = addWinding((*i)->getWinding());
		
		// Pass the Face& and the VertexList to create the polygon
		// and add the resulting Polygon structure to the list
		_polygons.push_back(addPolygon(*(*i), vertexList));
	}
	
	// Store the BrushStruc into the list
	_brushes.push_back(b);
}

void CollisionModel::setModel(const std::string& model) {
	_model = model;
}

// The friend stream insertion operator
std::ostream& operator<<(std::ostream& st, const CollisionModel& cm) {
	// Write the header
	st << "CM \"1.00\"\n\n0\n\n";
	
	st << "collisionModel \"" << cm._model.c_str() << "\" {\n";
	
	// Export the vertices
	st << "\tvertices { /* numVertices = */ " << cm._vertices.size() << "\n";
	for (VertexMap::const_iterator i = cm._vertices.begin(); 
		 i != cm._vertices.end(); 
		 i++) 
	{
		st << "\t/* " << i->first << " */ ";
		writeVector(st, i->second);
		st << "\n";
	}
	st << "\t}\n";
	
	// Export the edges
	st << "\tedges { /* numEdges = */ " << cm._edges.size() << "\n";
	for (EdgeMap::const_iterator i = cm._edges.begin(); 
		 i != cm._edges.end(); 
		 i++) 
	{
		st << "\t/* " << i->first << " */ ";
		st << "( " << i->second.from << " " << i->second.to << " ) ";
		st << "0 " << i->second.numVertices << "\n";
	}
	st << "\t}\n";
	
	// Write the kD-node ID (is always a leaf >> -1)
	st << "\tnodes {\n";
	st << "\t( -1 0 )\n";
	st << "\t}\n";
	
	// Export the polygons
	st << "\tpolygons /* polygonMemory = */ ";
	st << sizeof(Polygon)*cm._polygons.size();
	st << " {\n";
	for (unsigned int i = 0; i < cm._polygons.size(); i++) {
		st << "\t" << cm._polygons[i] << "\n";
	}
	st << "\t}\n";
	
	// Export the brushes, the header first
	st << "\tbrushes /* brushMemory = */ ";
	st << sizeof(BrushStruc)*cm._brushes.size();
	st << " {\n";
	
	// Now cycle through all the brushes
	for (unsigned int i = 0; i < cm._brushes.size(); i++) {
		st << "\t" << cm._brushes[i] << "\n";
	}
	st << "\t}\n";
	
	st << "}\n"; // end CollisionModel
	return st;
}

} // namespace cmutil
