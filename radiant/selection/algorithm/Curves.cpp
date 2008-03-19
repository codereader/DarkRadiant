#include "Curves.h"

#include "icurve.h"
#include "iundo.h"
#include "iradiant.h"
#include "gtkutil/dialog.h"
#include "selectionlib.h"

namespace selection {
	namespace algorithm {

// A basic functor doing an action to the curve
class CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) = 0;
};

// Appends a single control point to the visited curve
class CurveControlPointAppender :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.appendControlPoints(1);
		}
	}
};

// Removes the selected control points from the visited curve
class CurveControlPointRemover :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.removeSelectedControlPoints();
		}
	}
};

// Removes the selected control points from the visited curve
class CurveControlPointInserter :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.insertControlPointsAtSelected();
		}
	}
};

// Removes the selected control points from the visited curve
class CurveConverter :
	public CurveNodeProcessor
{
public:
	virtual void operator() (CurveNode& curve) {
		if (!curve.hasEmptyCurve()) {
			curve.convertCurveType();
		}
	}
};

/** greebo: This visits all selected curves and	calls 
 * 			the nominated Processor class using the 
 * 			CurveNode as argument.  
 */
class SelectedCurveVisitor : 
	public SelectionSystem::Visitor
{
	CurveNodeProcessor& _processor;
public:
	SelectedCurveVisitor(CurveNodeProcessor& processor) :
		_processor(processor)
	{}

	void visit(const scene::INodePtr& node) const {
		// Try to cast the instance onto a CurveNode
		CurveNodePtr curve = Node_getCurve(node);
		if (curve != NULL) {
			// Call the processor
			_processor(*curve);
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

void removeCurveControlPoints() {
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
		UndoableCommand command("curveRemoveControlPoints");
		
		// The functor object 
		CurveControlPointRemover remover;
		
		// Traverse the selection applying the functor
		GlobalSelectionSystem().foreachSelected(
			SelectedCurveVisitor(remover)
		);
	}
	else {
		gtkutil::errorDialog(
			"Can't remove curve points - no entities with curves selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}

void insertCurveControlPoints() {
	if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent ||
		GlobalSelectionSystem().ComponentMode() != SelectionSystem::eVertex)
	{
		gtkutil::errorDialog(
			"Can't insert curve points - must be in vertex editing mode.", 
			GlobalRadiant().getMainWindow()
		);
		return;
	}
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.entityCount > 0) {
		UndoableCommand command("curveInsertControlPoints");
		
		// The functor object 
		CurveControlPointInserter inserter;
		
		// Traverse the selection applying the functor
		GlobalSelectionSystem().foreachSelected(
			SelectedCurveVisitor(inserter)
		);
	}
	else {
		gtkutil::errorDialog(
			"Can't insert curve points - no entities with curves selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}

void convertCurveTypes() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.entityCount > 0) {
		UndoableCommand command("curveConvertType");
		
		// The functor object 
		CurveConverter converter;
		
		// Traverse the selection applying the functor
		GlobalSelectionSystem().foreachSelected(
			SelectedCurveVisitor(converter)
		);
	}
	else {
		gtkutil::errorDialog(
			"Can't convert curves - no entities with curves selected.", 
			GlobalRadiant().getMainWindow()
		);
	}
}
	
	} // namespace algorithm
} // namespace selection
