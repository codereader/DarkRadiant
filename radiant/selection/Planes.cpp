#include "Planes.h"

#include <functional>

void PlaneSelectableSelectPlanes::visit(const scene::INodePtr& node) const {
	// Skip hidden nodes
	if (!node->visible()) {
		return;
	}

	PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);
	if (planeSelectable != NULL) {
		planeSelectable->selectPlanes(_selector, _test, _selectedPlaneCallback);
	}
}

void PlaneSelectableSelectReversedPlanes::visit(const scene::INodePtr& node) const {
	// Skip hidden nodes
	if (!node->visible()) {
		return;
	}

	PlaneSelectablePtr planeSelectable = Node_getPlaneSelectable(node);
	if (planeSelectable != NULL) {
		planeSelectable->selectReversedPlanes(_selector, _selectedPlanes);
	}
}

void Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	PlaneSelectableSelectPlanes walker(selector, test, selectedPlaneCallback);
	GlobalSelectionSystem().foreachSelected(walker);
}

void Scene_forEachPlaneSelectable_selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	PlaneSelectableSelectReversedPlanes walker(selector, selectedPlanes);
	GlobalSelectionSystem().foreachSelected(walker);
}

bool Scene_forEachPlaneSelectable_selectPlanes(Selector& selector, SelectionTest& test) {
	SelectedPlaneSet selectedPlanes;

	Scene_forEachPlaneSelectable_selectPlanes(selector, test, std::bind(&SelectedPlaneSet::insert, &selectedPlanes, std::placeholders::_1));
	Scene_forEachPlaneSelectable_selectReversedPlanes(selector, selectedPlanes);

	return !selectedPlanes.empty();
}
