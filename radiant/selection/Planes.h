#ifndef PLANES_H_
#define PLANES_H_

#include "math/Plane3.h"
#include <set>
#include "iselectable.h"
#include "scenelib.h"

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
    if(plane.a < other.a) {
      return true;
    }
    if(other.a < plane.a) {
      return false;
    }

    if(plane.b < other.b) {
      return true;
    }
    if(other.b < plane.b) {
      return false;
    }
    
    if(plane.c < other.c) {
      return true;
    }
    if(other.c < plane.c) {
      return false;
    }

    if(plane.d < other.d) {
      return true;
    }
    
    if(other.d < plane.d) {
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
	typedef MemberCaller1<SelectedPlaneSet, const Plane3&, &SelectedPlaneSet::insert> InsertCaller;

	bool contains(const Plane3& plane) const {
		return _selectedPlanes.find(plane) != _selectedPlanes.end();
	}
};

bool Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test);
void Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
void Scene_forEachPlaneSelectable_selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);


#endif /*PLANES_H_*/
