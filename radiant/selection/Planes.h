#ifndef PLANES_H_
#define PLANES_H_

#include "math/Plane3.h"
#include <set>
#include "selectable.h"
#include "scenelib.h"

class PlaneSelectableSelectPlanes : public scene::Graph::Walker {
	Selector& _selector;
	SelectionTest& _test;
	PlaneCallback _selectedPlaneCallback;
public:
	PlaneSelectableSelectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback)
    	: _selector(selector), _test(test), _selectedPlaneCallback(selectedPlaneCallback) {}
	bool pre(const scene::Path& path, const scene::INodePtr& node) const;
};

class PlaneSelectableSelectReversedPlanes : public scene::Graph::Walker {
	Selector& _selector;
	const SelectedPlanes& _selectedPlanes;
public:
	PlaneSelectableSelectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes)
		: _selector(selector), _selectedPlanes(selectedPlanes) {}
	bool pre(const scene::Path& path, const scene::INodePtr& node) const;
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

inline void PlaneSet_insert(PlaneSet& self, const Plane3& plane) {
  self.insert(plane);
}

inline bool PlaneSet_contains(const PlaneSet& self, const Plane3& plane) {
  return self.find(plane) != self.end();
}

class SelectedPlaneSet : public SelectedPlanes
{
  PlaneSet _selectedPlanes;
public:
  bool empty() const {
    return _selectedPlanes.empty();
  }

  void insert(const Plane3& plane) {
    PlaneSet_insert(_selectedPlanes, plane);
  }
  bool contains(const Plane3& plane) const {
    return PlaneSet_contains(_selectedPlanes, plane);
  }
  typedef MemberCaller1<SelectedPlaneSet, const Plane3&, &SelectedPlaneSet::insert> InsertCaller;
};

bool Scene_forEachPlaneSelectable_selectPlanes(scene::Graph& graph, Selector& selector, SelectionTest& test);
void Scene_forEachPlaneSelectable_selectPlanes(scene::Graph& graph, Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
void Scene_forEachPlaneSelectable_selectReversedPlanes(scene::Graph& graph, Selector& selector, const SelectedPlanes& selectedPlanes);


#endif /*PLANES_H_*/
