#include "Curves.h"

#include "icurve.h"
#include "iundo.h"
#include "iradiant.h"
#include "gtkutil/Dialog.h"
#include "selectionlib.h"

namespace selection {
	namespace algorithm {

// A basic functor doing an action to the curve
class CurveInstanceProcessor
{
public:
	virtual void operator() (CurveInstance& curve) = 0;
};

// Appends a single control point to the visited curve
class CurveControlPointAppender :
	public CurveInstanceProcessor
{
public:
	virtual void operator() (CurveInstance& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.appendControlPoints(1);
		}
	}
};

// Removes the selected control points from the visited curve
class CurveControlPointRemover :
	public CurveInstanceProcessor
{
public:
	virtual void operator() (CurveInstance& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.removeSelectedControlPoints();
		}
	}
};

/** greebo: This visits all selected curves and	calls 
 * 			the nominated Processor class using the 
 * 			CurveInstance as argument.  
 */
class SelectedCurveVisitor : 
	public SelectionSystem::Visitor
{
	CurveInstanceProcessor& _processor;
public:
	SelectedCurveVisitor(CurveInstanceProcessor& processor) :
		_processor(processor)
	{}

	void visit(scene::Instance& instance) const {
		// Try to cast the instance onto a CurveInstance
		CurveInstance* curveInstance = Instance_getCurveInstance(instance);
		if (curveInstance != NULL) {
			// Call the processor
			_processor(*curveInstance);
		}
	}
};

void appendCurveControlPoint() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.entityCount > 0) {
		UndoableCommand command("curveAppendControlPoint");
		
		// The functor object 
		CurveControlPointAppender appender;
		
		// Traverse the selection applying the functor
		GlobalSelectionSystem().foreachSelected(
			SelectedCurveVisitor(appender)
		);
	}
	else {
		gtkutil::errorDialog(
			"Can't append curve point - no entities with curve selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}

void removeCurveControlPoint() {
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent ||
		GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex)
	{
		gtkutil::errorDialog(
			"Can't remove curve points - must be in vertex editing mode.", 
			GlobalRadiant().getMainWindow()
		);
		return;
	}
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.entityCount > 0) {
		UndoableCommand command("curveRemoveControlPoint");
		
		// The functor object 
		CurveControlPointRemover remover;
		
		// Traverse the selection applying the functor
		GlobalSelectionSystem().foreachSelected(
			SelectedCurveVisitor(remover)
		);
	}
	else {
		gtkutil::errorDialog(
			"Can't remove curve points - no entities with curve selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}
	
	} // namespace algorithm
} // namespace selection
