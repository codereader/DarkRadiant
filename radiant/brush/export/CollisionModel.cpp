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
		
	// Populate the FaceList
	for (Brush::const_iterator i = brush.begin(); i != brush.end(); i++) {
		const Winding& winding = (*i)->getWinding();
		
		// Check for degenerate winding, just to make sure
		if (winding.numpoints > 2) {
			b.point1 = winding[0].vertex;
			b.point2 = winding[winding.numpoints - 1].vertex;
		}
		else {
			globalErrorStream() << "CollisionModel: Warning: degenerate winding found.\n"; 
		}
		
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
	st << "test";
	return st;
}

} // namespace selection
