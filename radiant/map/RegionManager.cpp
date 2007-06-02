#include "RegionManager.h"

#include "iregistry.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "ientity.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ieventmanager.h"

#include "selectionlib.h"
#include "gtkutil/dialog.h"

#include "mainframe.h" // MainFrame_getWindow()
#include "select.h"

#include "brushmanip.h" // Construct_RegionBrushes()
#include "referencecache.h"
#include "RegionWalkers.h"
#include "MapFileManager.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"
#include "selection/algorithm/Primitives.h"

#include <boost/shared_ptr.hpp>

namespace map {

	namespace {
		typedef boost::shared_ptr<RegionManager> RegionManagerPtr;
		const std::string RKEY_PLAYER_START_ECLASS = "game/mapFormat/playerStartPoint";
	}

RegionManager::RegionManager() :
	_active(false)
{
	_worldMin = GlobalRegistry().getFloat("game/defaults/minWorldCoord");
	_worldMax = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
	
	for (int i = 0; i < 6; i++) {
		_brushes[i] = scene::INodePtr();
	}
}

bool RegionManager::isEnabled() const {
	return _active;
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

const AABB& RegionManager::getRegion() const {
	return _bounds;
}

void RegionManager::getMinMax(Vector3& regionMin, Vector3& regionMax) const {
	if (isEnabled()) {
		regionMin = _bounds.origin - _bounds.extents;
		regionMax = _bounds.origin + _bounds.extents;
	}
	else {
		// Set the region corners to the maximum available coords
		regionMin = Vector3(1,1,1)*_worldMin;
		regionMax = Vector3(1,1,1)*_worldMax;
	}
}

void RegionManager::setRegion(const AABB& aabb, bool autoEnable) {
	_bounds = aabb;
	
	if (autoEnable) {
		enable();
	}
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

void RegionManager::addRegionBrushes() {
	
	for (int i = 0; i < 6; i++) {
		// Create a new brush
		_brushes[i] = GlobalBrushCreator().createBrush();
		// Insert it into worldspawn
		Node_getTraversable(GlobalMap().findOrInsertWorldspawn())->insert(_brushes[i]);
	}
	
	// Obtain the size of the region (the corners)
	Vector3 min;
	Vector3 max;
	getMinMax(min, max);
	
	// Construct the region brushes (use the legacy GtkRadiant method, it works...)
	ConstructRegionBrushes(_brushes, min, max);
	
	// Get the player start EClass pointer
	const std::string eClassPlayerStart = GlobalRegistry().get(RKEY_PLAYER_START_ECLASS);
	IEntityClassPtr playerStart = GlobalEntityClassManager().findOrInsert(eClassPlayerStart, false);
	
	// Create the info_player_start entity
	_playerStart = GlobalEntityCreator().createEntity(playerStart);
	
	CamWnd* camWnd = GlobalCamera().getCamWnd();
	if (camWnd != NULL) { 
		// Obtain the camera origin = player start point
		Vector3 camOrigin = camWnd->getCameraOrigin();
		// Get the start angle of the player start point
		float angle = camWnd->getCameraAngles()[CAMERA_YAW];
		
		// Check if the camera origin is within the region
		if (aabb_intersects_point(_bounds, camOrigin)) {
			// Set the origin key of the playerStart entity
			Node_getEntity(_playerStart)->setKeyValue("origin", camOrigin);
			Node_getEntity(_playerStart)->setKeyValue("angle", floatToStr(angle));
		}
		else {
			gtkutil::errorDialog(
				"Warning: Camera not within region, can't set info_player_start.", 
				MainFrame_getWindow()
			);
		}
	}

  	// Insert the info_player_start into the scenegraph root
	Node_getTraversable(GlobalSceneGraph().root())->insert(_playerStart);
}

void RegionManager::removeRegionBrushes() {
	for (int i = 0; i < 6; i++) {
		// Remove the brushes from the scene
		if (_brushes[i] != NULL) {
			Node_getTraversable(GlobalMap().getWorldspawn())->erase(_brushes[i]);
			_brushes[i] = scene::INodePtr();
		}
	}
	
	if (_playerStart != NULL) {
		Node_getTraversable(GlobalSceneGraph().root())->erase(_playerStart);
	}
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

void RegionManager::setRegionFromSelection() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	// Check, if there is anything selected
	if (info.totalCount > 0) {
		if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {
			
			// Obtain the selection size (its min/max vectors)
			Vector3 min;
			Vector3 max;
			Select_GetBounds(min, max);
			
			// Set the region
			GlobalRegion().setRegion(AABB::createFromMinMax(min, max));
			
			// De-select all the selected items
			GlobalSelectionSystem().setSelectedAll(false);
			
			// Re-draw the scene
			SceneChangeNotify();
		}
		else {
			gtkutil::errorDialog("This command is not available in component mode.", 
								 MainFrame_getWindow());
			GlobalRegion().disable();
		}
	}
	else {
		gtkutil::errorDialog("Could not set Region: nothing selected.", MainFrame_getWindow());
		GlobalRegion().disable();
	}
}

void RegionManager::traverseRegion(scene::INodePtr root, const scene::Traversable::Walker& walker) {
	scene::TraversablePtr traversable = Node_getTraversable(root);
	
	if (traversable != NULL) {
		// Pass the given Walker on to the ExcludeWalker, 
		// which calls the walker.pre() and .post() methods if the visited item is regioned.
		traversable->traverse(ExcludeNonRegionedWalker(walker));
	}
}

void RegionManager::saveRegion() {
	// Query the desired filename from the user
	std::string filename = map::MapFileManager::getMapFilename(false, "Export region");
	
	if (!filename.empty()) {
		// Filename is ok, start preparation
		
		// Save the old region
		AABB oldRegionAABB = GlobalRegion().getRegion();
		
		// Now check for the effective bounds so that all visible items are included
		AABB visibleBounds = map::getVisibleBounds();
		
		// Set the region bounds, but don't traverse the graph!
		GlobalRegion().setRegion(visibleBounds, false);
		
		// Add the region brushes
		GlobalRegion().addRegionBrushes();
		
		// Substract the origin from child primitives (of entities like func_static)
		selection::algorithm::removeOriginFromChildPrimitives();
		
		// Save the map and pass the RegionManager::traverseRegion functor 
		// that assures that only regioned items are traversed
		MapResource_saveFile(Map::getFormatForFile(filename),
							 GlobalSceneGraph().root(),
  							 RegionManager::traverseRegion,
  							 filename.c_str());
		
		// Add the origin to all the children of func_static, etc.
		selection::algorithm::addOriginToChildPrimitives();
		
		// Remove the region brushes
		GlobalRegion().removeRegionBrushes();
		
		// Set the region AABB back to the state before saving
		GlobalRegion().setRegion(oldRegionAABB, false);
	}
}

void RegionManager::initialiseCommands() {
	GlobalEventManager().addCommand("SaveRegion", FreeCaller<RegionManager::saveRegion>());
	GlobalEventManager().addCommand("RegionOff", FreeCaller<RegionManager::disableRegion>());
	GlobalEventManager().addCommand("RegionSetXY", FreeCaller<RegionManager::setRegionXY>());
	GlobalEventManager().addCommand("RegionSetBrush", FreeCaller<RegionManager::setRegionFromBrush>());
	GlobalEventManager().addCommand("RegionSetSelection", FreeCaller<RegionManager::setRegionFromSelection>());
}

} // namespace map

map::RegionManager& GlobalRegion() {
	static map::RegionManagerPtr _regionManager;
	
	if (_regionManager == NULL) {
		_regionManager = map::RegionManagerPtr(new map::RegionManager);
	}
	
	return *_regionManager;
}
