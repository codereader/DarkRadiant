#include "Planes.h"

#include "math/Plane3.h"
#include <set>
#include <functional>
#include "selection/SelectedPlaneSet.h"

namespace selection
{

namespace algorithm
{

void testSelectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
{
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		// Skip hidden nodes
		if (!node->visible()) return;

		PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);

		if (planeSelectable)
		{
			planeSelectable->selectPlanes(selector, test, selectedPlaneCallback);
		}
	});
}

void testSelectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		// Skip hidden nodes
		if (!node->visible()) return;

		PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);

		if (planeSelectable)
		{
			planeSelectable->selectReversedPlanes(selector, selectedPlanes);
		}
	});
}

bool testSelectPlanes(Selector& selector, SelectionTest& test)
{
	SelectedPlaneSet selectedPlanes;

	testSelectPlanes(selector, test, std::bind(&SelectedPlaneSet::insert, &selectedPlanes, std::placeholders::_1));
	testSelectReversedPlanes(selector, selectedPlanes);

	return !selectedPlanes.empty();
}

} // namespace

} // namespace
