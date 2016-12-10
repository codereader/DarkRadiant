#include "Planes.h"

#include "math/Plane3.h"
#include <set>
#include <functional>

namespace selection
{

namespace algorithm
{

// Considers all front planes of selected planeselectables
class PlaneSelectableSelectPlanes :
	public SelectionSystem::Visitor
{
	Selector& _selector;
	SelectionTest& _test;
	PlaneCallback _selectedPlaneCallback;
public:
	PlaneSelectableSelectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) :
		_selector(selector),
		_test(test),
		_selectedPlaneCallback(selectedPlaneCallback)
	{}

	void visit(const scene::INodePtr& node) const;
};

// Considers all back planes of selected planeselectables
class PlaneSelectableSelectReversedPlanes :
	public SelectionSystem::Visitor
{
	Selector& _selector;
	const SelectedPlanes& _selectedPlanes;
public:
	PlaneSelectableSelectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) :
		_selector(selector),
		_selectedPlanes(selectedPlanes)
	{}

	void visit(const scene::INodePtr& node) const;
};

void PlaneSelectableSelectPlanes::visit(const scene::INodePtr& node) const
{
	// Skip hidden nodes
	if (!node->visible())
	{
		return;
	}

	PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);

	if (planeSelectable)
	{
		planeSelectable->selectPlanes(_selector, _test, _selectedPlaneCallback);
	}
}

void PlaneSelectableSelectReversedPlanes::visit(const scene::INodePtr& node) const
{
	// Skip hidden nodes
	if (!node->visible())
	{
		return;
	}

	PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);

	if (planeSelectable)
	{
		planeSelectable->selectReversedPlanes(_selector, _selectedPlanes);
	}
}

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

void Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
{
	PlaneSelectableSelectPlanes walker(selector, test, selectedPlaneCallback);
	GlobalSelectionSystem().foreachSelected(walker);
}

void Scene_forEachPlaneSelectable_selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
{
	PlaneSelectableSelectReversedPlanes walker(selector, selectedPlanes);
	GlobalSelectionSystem().foreachSelected(walker);
}

bool testSelectPlanes(Selector& selector, SelectionTest& test)
{
	SelectedPlaneSet selectedPlanes;

	Scene_forEachPlaneSelectable_selectPlanes(selector, test, std::bind(&SelectedPlaneSet::insert, &selectedPlanes, std::placeholders::_1));
	Scene_forEachPlaneSelectable_selectReversedPlanes(selector, selectedPlanes);

	return !selectedPlanes.empty();
}

} // namespace

} // namespace
