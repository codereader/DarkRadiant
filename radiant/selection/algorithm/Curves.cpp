#include "Curves.h"

#include "icurve.h"
#include "iundo.h"
#include "iradiant.h"
#include "gtkutil/Dialog.h"
#include "selectionlib.h"

namespace selection {
	namespace algorithm {
	
class CurveControlPointAppender : 
	public SelectionSystem::Visitor
{
public:
	void visit(scene::Instance& instance) const {
		// Try to cast the instance onto a CurveInstance
		CurveInstance* curveInstance = Instance_getCurveInstance(instance);
		if (curveInstance != NULL && !curveInstance->hasEmptyCurve()) {
			// Append one control point to the curve
			curveInstance->appendControlPoints(1);
		}
	}
};

void appendCurveControlPoint() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.entityCount > 0) {
		UndoableCommand command("curveAppendControlPoint");
		
		GlobalSelectionSystem().foreachSelected(CurveControlPointAppender());
	}
	else {
		gtkutil::errorDialog(
			"Can't append curve point - no entities with curve selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}
	
	} // namespace algorithm
} // namespace selection
