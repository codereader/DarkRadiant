#include "Planes.h"

#include "math/Plane3.h"
#include <set>
#include <functional>

namespace selection
{

namespace algorithm
{

class PlaneLess
{
public:
	bool operator()(const Plane3& plane, const Plane3& other) const
	{
		if (plane.normal().x() < other.normal().x()) {
			return true;
		}
		if (other.normal().x() < plane.normal().x()) {
			return false;
		}

		if (plane.normal().y() < other.normal().y()) {
			return true;
		}
		if (other.normal().y() < plane.normal().y()) {
			return false;
		}

		if (plane.normal().z() < other.normal().z()) {
			return true;
		}
		if (other.normal().z() < plane.normal().z()) {
			return false;
		}

		if (plane.dist() < other.dist()) {
			return true;
		}

		if (other.dist() < plane.dist()) {
			return false;
		}

		return false;
	}
};

class SelectedPlaneSet : public SelectedPlanes
{
private:
	typedef std::set<Plane3, PlaneLess> PlaneSet;
	PlaneSet _selectedPlanes;

public:
	bool empty() const
	{
		return _selectedPlanes.empty();
	}

	void insert(const Plane3& plane)
	{
		_selectedPlanes.insert(plane);
	}

	bool contains(const Plane3& plane) const
	{
		return _selectedPlanes.find(plane) != _selectedPlanes.end();
	}
};

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
