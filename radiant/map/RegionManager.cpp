#include "RegionManager.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include "iselection.h"

#include "selectionlib.h"
#include "gtkutil/dialog.h"

#include "mainframe.h" // MainFrame_getWindow()

#include "RegionWalkers.h"
#include "xyview/GlobalXYWnd.h"

#include <boost/shared_ptr.hpp>

namespace map {

	namespace {
		typedef boost::shared_ptr<RegionManager> RegionManagerPtr;
	}

RegionManager::RegionManager() :
	_active(false)
{
	_worldMin = GlobalRegistry().getFloat("game/defaults/minWorldCoord");
	_worldMax = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
}

void RegionManager::disable() {
	_active = false;

	_bounds = AABB::createFromMinMax(Vector3(1,1,1)*_worldMin, Vector3(1,1,1)*_worldMax);
	// Create a 64 unit border from the max world coord
	_bounds.extents -= Vector3(1,1,1)*64;
	
	// Disable the exclude bit on all the scenegraph nodes
	GlobalSceneGraph().traverse(ExcludeAllWalker(false));
}

void RegionManager::enable() {
	if (!_bounds.isValid()) {
		return;
	}
	
	_active = true;
	
	// Show all elements within the current region / hide the outsiders
	GlobalSceneGraph().traverse(ExcludeRegionedWalker(false, _bounds));
}

void RegionManager::setRegion(const AABB& aabb) {
	_bounds = aabb;
	
	enable();
}

void RegionManager::setRegionFromXY(Vector2 topLeft, Vector2 lowerRight) {
	// Reset the regioning before doing anything else
	disable();

	// Make sure the given coordinates are correctly sorted 	
	for (unsigned int i = 0; i < 2; i++) {
		if (lowerRight[i] < topLeft[i]) {
			std::swap(lowerRight[i], topLeft[i]);
		}
	}

	Vector3 min(topLeft[0], topLeft[1], _worldMin + 64);
	Vector3 max(lowerRight[0], lowerRight[1], _worldMax - 64);
	
	// Create an AABB out of the given vectors and set the region (enable() is triggered)
	setRegion(AABB::createFromMinMax(min, max));
}

// Static members (used as command targets for EventManager)

void RegionManager::disableRegion() {
	GlobalRegion().disable();
	SceneChangeNotify();
}

void RegionManager::setRegionXY() {
	// Obtain the current XY orthoview, if there is one
	XYWnd* xyWnd = GlobalXYWnd().getView(XY);
	
	if (xyWnd != NULL) {
		Vector2 topLeft(
			xyWnd->getOrigin()[0] - 0.5f * xyWnd->getWidth() / xyWnd->getScale(),
			xyWnd->getOrigin()[1] - 0.5f * xyWnd->getHeight() / xyWnd->getScale()
		);
		
		Vector2 lowerRight(
			xyWnd->getOrigin()[0] + 0.5f * xyWnd->getWidth() / xyWnd->getScale(),
			xyWnd->getOrigin()[1] + 0.5f * xyWnd->getHeight() / xyWnd->getScale()
		);
		
		// Set the bounds from the calculated XY rectangle
		GlobalRegion().setRegionFromXY(topLeft, lowerRight);
	}
	else {
		gtkutil::errorDialog("Could not set Region: XY Top View not found.", MainFrame_getWindow());
		GlobalRegion().disable();
	}
	SceneChangeNotify();
}

void RegionManager::setRegionFromBrush() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// Check, if exactly one brush is selected
	if (info.brushCount == 1 && info.totalCount == 1) {
		// Get the selected instance
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		// Set the bounds of the region to the selection's extents
		GlobalRegion().setRegion(instance.worldAABB());
		
		// Delete the currently selected brush (undoable command)
		deleteSelection();
		
		SceneChangeNotify();
	}
	else {
		gtkutil::errorDialog("Could not set Region: please select a single Brush.", MainFrame_getWindow());
		GlobalRegion().disable();
	}
}

void RegionManager::saveRegion() {
	
}

} // namespace map

map::RegionManager& GlobalRegion() {
	static map::RegionManagerPtr _regionManager;
	
	if (_regionManager == NULL) {
		_regionManager = map::RegionManagerPtr(new map::RegionManager);
	}
	
	return *_regionManager;
}
