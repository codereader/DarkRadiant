#include "CollisionModel.h"

#include "stream/textstream.h"
#include "itextstream.h"
#include "iselection.h"
#include "ientity.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "brush/Brush.h"
#include "winding.h"

namespace selection {

	namespace {
		const std::string ECLASS_CLIPMODEL = "func_clipmodel";
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

void CollisionModel::addPolygon(const VertexList& vertexList) {
	// Cycle from the beginning to the end-1 and add the edges 
	for (unsigned int i = 0; i < vertexList.size()-1; i++) {
		
	}
}

void CollisionModel::addWinding(const Winding& winding) {
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
	
	// Now that all edges are added, create the polygon
	addPolygon(vertexList);
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
		
		// Parse the winding of this Face for vertices/edges/polygons
		addWinding((*i)->getWinding());
	}
	
	// Store the BrushStruc into the list
	_brushes.push_back(b);
}

bool CollisionModel::isValid() const {
	return true;
}

void CollisionModel::createFromSelection() {
	globalOutputStream() << "CollisionModel::createFromSelection started.\n";
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount == info.entityCount && info.totalCount == 1) {
		// Retrieve the node, instance and entity
		scene::Instance& entityInstance = GlobalSelectionSystem().ultimateSelected();
		scene::Node& entityNode = entityInstance.path().top();
		Entity* entity = Node_getEntity(entityNode);
		
		if (entity != NULL && entity->getKeyValue("classname") == ECLASS_CLIPMODEL) {
			// Try to retrieve the group node
			scene::GroupNode* groupNode = Node_getGroupNode(entityNode);
			
			// Remove the entity origin from the brushes
			if (groupNode != NULL) {
				groupNode->removeOriginFromChildren();
				
				// Deselect the instance
				Instance_setSelected(entityInstance, false);
				
				// Select all the child nodes
				Node_getTraversable(entityNode)->traverse(
					SelectChildren(entityInstance.path())
				);
				
				BrushPtrVector brushes = algorithm::getSelectedBrushes();
			
				// Create a new collisionmodel on the heap using a shared_ptr
				CollisionModelPtr cm(new CollisionModel());
			
				globalOutputStream() << "Brushes found: " << brushes.size() << "\n";
			
				// Add all the brushes to the collision model
				for (unsigned int i = 0; i < brushes.size(); i++) {
					cm->addBrush(*brushes[i]);
				}
				
				// De-select the child brushes
				GlobalSelectionSystem().setSelectedAll(false);
				
				// Re-add the origin to the brushes
				groupNode->addOriginToChildren();
			
				// Re-select the instance
				Instance_setSelected(entityInstance, true);
			
				// Output the data of the cm (debug)
				std::cout << *cm;
			}
		}
		else {
			globalErrorStream() << "Cannot export, wrong entity (func_clipmodel required).\n";
		}
	}
	else {
		globalErrorStream() << "Cannot export, create and selecte a func_clipmodel entity.\n";
	}
}

// The friend stream insertion operator
std::ostream& operator<<(std::ostream& st, const CollisionModel& cm) {
	unsigned int counter = 0;
	
	st << "CM \"1.00\"\n\n0\n\n";
	st << "collisionModel \"models/props/\" {\n";
	
	// Export the vertices
	st << "\tvertices { /* numVertices = */ " << cm._vertices.size() << "\n";
	for (CollisionModel::VertexMap::const_iterator i = cm._vertices.begin(); 
		 i != cm._vertices.end(); 
		 i++, counter++) 
	{
		st << "\t/* " << counter << " */ ( " << i->second[0] << " " << i->second[1] << " " << i->second[2] << " )\n";
	}
	st << "\t}\n";
	
	// Export the edges
	st << "\tedges { /* numEdges = */ " << cm._edges.size() << "\n";
	counter = 0;
	for (CollisionModel::EdgeMap::const_iterator i = cm._edges.begin(); 
		 i != cm._edges.end(); 
		 i++, counter++) 
	{
		st << "\t/* " << counter << " */ ";
		st << "( " << i->second.from << " " << i->second.to << " ) ";
		st << "0 " << i->second.numVertices << "\n";
	}
	st << "\t}\n";
	
	// Write the kD-node ID (is always a leaf >> -1)
	st << "\tnodes {\n";
	st << "\t( -1 0 )\n";
	st << "\t}\n";
	
	// Export the polygons
	
	// Export the brushes, the header first
	st << "\tbrushes { /* brushMemory = */ ";
	st << sizeof(CollisionModel::BrushStruc)*cm._brushes.size();
	st << " {\n";
	
	// Now cycle through all the brushes
	for (unsigned int i = 0; i < cm._brushes.size(); i++) {
		CollisionModel::BrushStruc b = cm._brushes[i];
		
		st << "\t";
		st << cm._brushes[i].numFaces << " {\n";
		
		// Write all the planes
		for (unsigned int p = 0; p < cm._brushes[i].planes.size(); p++) {
			st << "\t\t( ";
			st << b.planes[p].normal()[0] << " ";
			st << b.planes[p].normal()[1] << " ";
			st << b.planes[p].normal()[2] << " ) ";
			st << b.planes[p].dist() << "\n";
		}
		
		st << "\t} ";
		
		// Write the two AABB vertices
		st << " ( ";
		st << b.min[0] << " ";
		st << b.min[1] << " ";
		st << b.min[2] << " ) ";
		
		st << "( ";
		st << b.max[0] << " ";
		st << b.max[1] << " ";
		st << b.max[2] << " ) ";
		
		// Now append the "solid"
		st << "\"solid\"\n"; // TODO: Add correct response type (solid, opaque, trigger, etc.)
	}
	st << "\t}\n";
	
	st << "}\n"; // end CollisionModel
	return st;
}

} // namespace selection
