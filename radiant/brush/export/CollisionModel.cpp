#include "CollisionModel.h"

#include "stream/textstream.h"
#include "itextstream.h"
#include "iselection.h"
#include "selectionlib.h"
#include "brush/Brush.h"
#include "winding.h"

namespace selection {

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
	
	if (info.totalCount == info.brushCount && info.totalCount > 0) {
		// Create a new collisionmodel on the heap using a shared_ptr
		CollisionModelPtr cm(new CollisionModel());
		
		BrushPtrVector brushes = algorithm::getSelectedBrushes();
		
		for (unsigned int i = 0; i < brushes.size(); i++) {
			// Add the selected brush to the collision model
			cm->addBrush(*brushes[i]);
		}
		
		// Output the data of the cm (debug)
		std::cout << *cm;
	}
	else {
		globalErrorStream() << "Sorry, only brushes can be exported to an CM file.\n";
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
	
	// Export the edges (including the (0 0) edge there are n+1)
	st << "\tedges { /* numEdges = */ " << cm._edges.size()+1 << "\n";
	
	// Write the first edge (seems to be always 0 0)
	st << "\t/* 0 */ ( 0 0 ) 0 0\n";
	
	// Now start with edge #1
	counter = 1;
	for (CollisionModel::EdgeMap::const_iterator i = cm._edges.begin(); 
		 i != cm._edges.end(); 
		 i++, counter++) 
	{
		st << "\t/* " << counter << " */ ( " << i->second.from << " " << i->second.to << " ) 0 2\n";
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
		st << "\"solid\"\n";
	}
	st << "\t}\n";
	
	st << "}\n"; // end CollisionModel
	return st;
}

} // namespace selection
