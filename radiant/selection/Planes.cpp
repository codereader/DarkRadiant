#include "Planes.h"

bool PlaneSelectableSelectPlanes::pre(const scene::Path& path, scene::Instance& instance) const {
    if(path.top().get().visible())
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0 && selectable->isSelected())
      {
        PlaneSelectable* planeSelectable = Instance_getPlaneSelectable(instance);
        if(planeSelectable != 0)
        {
          planeSelectable->selectPlanes(_selector, _test, _selectedPlaneCallback);
        }
      }
    }
    return true; 
}

bool PlaneSelectableSelectReversedPlanes::pre(const scene::Path& path, scene::Instance& instance) const {
    if(path.top().get().visible())
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0 && selectable->isSelected())
      {
        PlaneSelectable* planeSelectable = Instance_getPlaneSelectable(instance);
        if(planeSelectable != 0)
        {
          planeSelectable->selectReversedPlanes(_selector, _selectedPlanes);
        }
      }
    }
    return true; 
}

void Scene_forEachPlaneSelectable_selectPlanes(scene::Graph& graph, Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
  graph.traverse(PlaneSelectableSelectPlanes(selector, test, selectedPlaneCallback));
}

void Scene_forEachPlaneSelectable_selectReversedPlanes(scene::Graph& graph, Selector& selector, const SelectedPlanes& selectedPlanes) {
  graph.traverse(PlaneSelectableSelectReversedPlanes(selector, selectedPlanes));
}

bool Scene_forEachPlaneSelectable_selectPlanes(scene::Graph& graph, Selector& selector, SelectionTest& test) {
  SelectedPlaneSet selectedPlanes;

  Scene_forEachPlaneSelectable_selectPlanes(graph, selector, test, SelectedPlaneSet::InsertCaller(selectedPlanes));
  Scene_forEachPlaneSelectable_selectReversedPlanes(graph, selector, selectedPlanes);

  return !selectedPlanes.empty();
}

