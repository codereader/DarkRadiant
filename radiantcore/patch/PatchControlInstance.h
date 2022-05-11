#pragma once

#include "ObservedSelectable.h"
#include "iselectiontest.h"
#include "PatchControl.h"

/* greebo: a PatchControlInstance basically consists of two parts: an ObservedSelectable and a PatchControl itself
 *
 * The PatchControl is a struct defined in ipatch.h and consists of a vertex and texture coordinates.
 *
 * The ObservedSelectable is inherited to inform the SelectionSystem about the selection changes. Everytime the
 * selection state is altered, it calls the <observer> it has been passed in its constructor.
 */
class PatchControlInstance :
	public selection::ObservedSelectable
{
public:
	// The pointer to the actual PatchControl structure
	PatchControl& control;

	// Constructor
	// It takes a reference to a PatchControl and the SelectionChanged callback as argument
	// The observer/callback usually points back to the PatchNode::selectedChangeComponent member method
	PatchControlInstance(PatchControl& ctrl, const SelectionChangedSlot& observer) :
		selection::ObservedSelectable(observer),
		control(ctrl)
	{}

	PatchControlInstance& operator=(const PatchControlInstance& other)
	{
		control = other.control;
		return *this;
	}

	// Check if the control is selected by using the given SelectionTest
	void testSelect(Selector& selector, SelectionTest& test)
	{
		SelectionIntersection best;
		test.TestPoint(control.vertex, best);

		// If there is a control point that can be selected, add the Selectable to the selector
		if (best.isValid())
		{
			selector.addWithIntersection(*this, best);
		}
	}

	// Snaps the control vertex to the grid
	void snapto(float snap)
	{
		control.vertex.snap(snap);
	}
};
