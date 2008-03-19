#ifndef PATCHCONTROLINSTANCE_H_
#define PATCHCONTROLINSTANCE_H_

#include "selectionlib.h"
#include "selectable.h"
#include "PatchControl.h"

/* greebo: a PatchControlInstance basically consists of two parts: an ObservedSelectable and a PatchControl itself
 * 
 * The PatchControl is a struct defined in ipatch.h and consists of a vertex and texture coordinates.
 * 
 * The ObservedSelectable is needed to inform the SelectionSystem about the selection changes. Everytime the
 * selection state is altered, it calls the <observer> it has been passed in its constructor.
 * 
 * The ObservedSelectable is then passing itself to the PatchNode and from there further on to the SelectionSystem,
 * so that everything "keeps track of itself" (counters for example). 
 */
class PatchControlInstance {
public:
	// The pointer to the actual PatchControl structure
	PatchControl* m_ctrl;
	// The Selectable
	ObservedSelectable m_selectable;

	// Constructor
	// It takes a pointer to a PatchControl and the SelectionChanged callback as argument
	// The observer/callback usually points back to the PatchNode::selectedChangeComponent member method
	PatchControlInstance(PatchControl* ctrl, const SelectionChangeCallback& observer) : 
		m_ctrl(ctrl), 
		m_selectable(observer)
	{
	}

	// Check if the control is selected by using the given SelectionTest
	void testSelect(Selector& selector, SelectionTest& test) {
		SelectionIntersection best;
		test.TestPoint(m_ctrl->m_vertex, best);
		
		// If there is a control point that can be selected, add the Selectable to the selector
		if (best.valid()) {
			Selector_add(selector, m_selectable, best);
		}
	}
	
	// Snaps the control vertex to the grid
	void snapto(float snap) {
		vector3_snap(m_ctrl->m_vertex, snap);
	}
};

#endif /*PATCHCONTROLINSTANCE_H_*/
