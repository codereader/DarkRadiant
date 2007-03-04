#include "CollisionModel.h"

#include "stream/textstream.h"
#include "itextstream.h"
#include "iselection.h"
#include "selectionlib.h"
#include "brush/Brush.h"

namespace selection {

void CollisionModel::addBrush(Brush& brush) {
	
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
	}
	else {
		globalErrorStream() << "Sorry, only brushes can be exported to an CM file.\n";
	}
}
	
} // namespace selection
