#include "Planes.h"

bool PlaneSelectableSelectPlanes::pre(const scene::Path& path, const scene::INodePtr& node) const {
	if (node->visible()) {
		if (Node_isSelected(node)) {
			PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);
			if (planeSelectable != NULL) {
				planeSelectable->selectPlanes(_selector, _test, _selectedPlaneCallback);
			}
		}
	}
    return true; 
}

bool PlaneSelectableSelectReversedPlanes::pre(const scene::Path& path, const scene::INodePtr& node) const {
	if (node->visible()) {
		if (Node_isSelected(node)) {
			PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);
			if (planeSelectable != NULL) {
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

