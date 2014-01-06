#ifndef PLANES_H_
#define PLANES_H_

#include "math/Plane3.h"
#include <set>
#include "iselectiontest.h"

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

class PlaneLess
{
public:
  bool operator()(const Plane3& plane, const Plane3& other) const {
    if(plane.normal().x() < other.normal().x()) {
      return true;
    }
    if(other.normal().x() < plane.normal().x()) {
      return false;
    }

    if(plane.normal().y() < other.normal().y()) {
      return true;
    }
    if(other.normal().y() < plane.normal().y()) {
      return false;
    }

    if(plane.normal().z() < other.normal().z()) {
      return true;
    }
    if(other.normal().z() < plane.normal().z()) {
      return false;
    }

    if(plane.dist() < other.dist()) {
      return true;
    }

    if(other.dist() < plane.dist()) {
      return false;
    }

    return false;
  }
};

typedef std::set<Plane3, PlaneLess> PlaneSet;

class SelectedPlaneSet : public SelectedPlanes
{
	PlaneSet _selectedPlanes;
public:
	bool empty() const {
		return _selectedPlanes.empty();
	}

	void insert(const Plane3& plane) {
		_selectedPlanes.insert(plane);
	}

	bool contains(const Plane3& plane) const {
		return _selectedPlanes.find(plane) != _selectedPlanes.end();
	}
};

bool Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test);
void Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
void Scene_forEachPlaneSelectable_selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);


#endif /*PLANES_H_*/
