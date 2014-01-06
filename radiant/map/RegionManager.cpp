#include "RegionManager.h"

#include "i18n.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "ientity.h"
#include "ieclass.h"
#include "imainframe.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ieventmanager.h"

#include "scenelib.h"
#include "selectionlib.h"
#include "shaderlib.h"
#include "gamelib.h"
#include "string/string.h"
#include "gtkutil/dialog/MessageBox.h"

#include "registry/registry.h"
#include "brush/Brush.h"
#include "RegionWalkers.h"
#include "MapFileManager.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"
#include "ui/mru/MRU.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/General.h"
#include "map/MapResource.h"

#include <boost/shared_ptr.hpp>

namespace map {

    namespace {
        typedef boost::shared_ptr<RegionManager> RegionManagerPtr;
        const std::string GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";

        class AABBCollectorVisible :
            public scene::NodeVisitor
        {
            AABB& _targetAABB;
        public:
            AABBCollectorVisible(AABB& targetAABB) :
                _targetAABB(targetAABB)
            {}

            bool pre(const scene::INodePtr& node)
            {
                if (node->visible())
                {
                    _targetAABB.includeAABB(node->worldAABB());
                }

                return true;
            }
        };

        AABB getVisibleBounds()
        {
            AABB returnValue;
            AABBCollectorVisible collector(returnValue);

            GlobalSceneGraph().root()->traverse(collector);

            return returnValue;
        }
    }

RegionManager::RegionManager() :
    _active(false)
{
    _worldMin = game::current::getValue<float>("/defaults/minWorldCoord");
    _worldMax = game::current::getValue<float>("/defaults/maxWorldCoord");

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
    ExcludeAllWalker walker(false);
    GlobalSceneGraph().root()->traverse(walker);
}

void RegionManager::enable() {
    if (!_bounds.isValid()) {
        return;
    }

    _active = true;

    // Show all elements within the current region / hide the outsiders
    ExcludeRegionedWalker walker(false, _bounds);
    GlobalSceneGraph().root()->traverse(walker);
}

void RegionManager::clear()
{
    for (int i = 0; i < 6; i++) {
        _brushes[i] = scene::INodePtr();
    }

    _playerStart.reset();
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

void RegionManager::addRegionBrushes()
{
    for (int i = 0; i < 6; i++) {
        // Create a new brush
        _brushes[i] = GlobalBrushCreator().createBrush();
        // Insert it into worldspawn
        scene::addNodeToContainer(_brushes[i], GlobalMap().findOrInsertWorldspawn());
    }

    // Obtain the size of the region (the corners)
    Vector3 min;
    Vector3 max;
    getMinMax(min, max);

    // Construct the region brushes (use the legacy GtkRadiant method, it works...)
    constructRegionBrushes(_brushes, min, max);

    // Get the player start EClass pointer
    const std::string eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);
    IEntityClassPtr playerStart = GlobalEntityClassManager().findOrInsert(eClassPlayerStart, false);

    // Create the info_player_start entity
    _playerStart = GlobalEntityCreator().createEntity(playerStart);

    CamWndPtr camWnd = GlobalCamera().getActiveCamWnd();

    if (camWnd != NULL) {
        // Obtain the camera origin = player start point
        Vector3 camOrigin = camWnd->getCameraOrigin();
        // Get the start angle of the player start point
        float angle = camWnd->getCameraAngles()[CAMERA_YAW];

        // Check if the camera origin is within the region
        if (_bounds.intersects(camOrigin))
        {
            // Set the origin key of the playerStart entity
            _playerStart->getEntity().setKeyValue("origin",
                                                  string::to_string(camOrigin));
            _playerStart->getEntity().setKeyValue("angle",
                                                  string::to_string(angle));
        }
        else {
            gtkutil::MessageBox::ShowError(
                _("Warning: Camera not within region, can't set info_player_start."),
                GlobalMainFrame().getTopLevelWindow()
            );
        }
    }

    // Insert the info_player_start into the scenegraph root
    GlobalSceneGraph().root()->addChildNode(_playerStart);
}

void RegionManager::constructRegionBrushes(scene::INodePtr brushes[6], const Vector3& region_mins, const Vector3& region_maxs)
{
	const float THICKNESS = 10;

	{
		// set mins
		Vector3 mins(region_mins[0]-THICKNESS, region_mins[1]-THICKNESS, region_mins[2]-THICKNESS);

		// vary maxs
		for (std::size_t i = 0; i < 3; i++)
		{
			Vector3 maxs(region_maxs[0]+THICKNESS, region_maxs[1]+THICKNESS, region_maxs[2]+THICKNESS);
			maxs[i] = region_mins[i];

			Brush& brush = *Node_getBrush(brushes[i]);
			brush.constructCuboid(AABB::createFromMinMax(mins, maxs),
      							texdef_name_default(),
      							TextureProjection());
		}
	}

	{
		// set maxs
		Vector3 maxs(region_maxs[0]+THICKNESS, region_maxs[1]+THICKNESS, region_maxs[2]+THICKNESS);

		// vary mins
		for (std::size_t i = 0; i < 3; i++)
		{
			Vector3 mins(region_mins[0]-THICKNESS, region_mins[1]-THICKNESS, region_mins[2]-THICKNESS);
			mins[i] = region_maxs[i];
			Brush& brush = *Node_getBrush(brushes[i+3]);
	  
			brush.constructCuboid(AABB::createFromMinMax(mins, maxs),
      							texdef_name_default(),
      							TextureProjection());
		}
	}
}

void RegionManager::removeRegionBrushes() {
    for (int i = 0; i < 6; i++) {
        // Remove the brushes from the scene
        if (_brushes[i] != NULL) {
            GlobalMap().getWorldspawn()->removeChildNode(_brushes[i]);
            _brushes[i] = scene::INodePtr();
        }
    }

    if (_playerStart)
    {
        GlobalSceneGraph().root()->removeChildNode(_playerStart);
    }
}

// Static members (used as command targets for EventManager)

void RegionManager::disableRegion(const cmd::ArgumentList& args) {
    GlobalRegion().disable();
    SceneChangeNotify();
}

void RegionManager::setRegionXY(const cmd::ArgumentList& args) {
    // Obtain the current XY orthoview, if there is one
    XYWndPtr xyWnd = GlobalXYWnd().getView(XY);

    if (xyWnd) {
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
        gtkutil::MessageBox::ShowError(
            _("Could not set Region: XY Top View not found."),
            GlobalMainFrame().getTopLevelWindow());
        GlobalRegion().disable();
    }
    SceneChangeNotify();
}

void RegionManager::setRegionFromBrush(const cmd::ArgumentList& args) {
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // Check, if exactly one brush is selected
    if (info.brushCount == 1 && info.totalCount == 1) {
        // Get the selected node
        const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

        // Set the bounds of the region to the selection's extents
        GlobalRegion().setRegion(node->worldAABB());

        // Delete the currently selected brush (undoable command)
        {
            UndoableCommand undo("deleteSelected");
            selection::algorithm::deleteSelection();
        }

        SceneChangeNotify();
    }
    else {
        gtkutil::MessageBox::ShowError(
            _("Could not set Region: please select a single Brush."),
            GlobalMainFrame().getTopLevelWindow());
        GlobalRegion().disable();
    }
}

void RegionManager::setRegionFromSelection(const cmd::ArgumentList& args) {
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // Check, if there is anything selected
    if (info.totalCount > 0) {
        if (GlobalSelectionSystem().Mode() != SelectionSystem::eComponent) {

            // Obtain the selection size (its min/max vectors)
            AABB regionBounds = GlobalSelectionSystem().getWorkZone().bounds;

            // Set the region
            GlobalRegion().setRegion(regionBounds);

            // De-select all the selected items
            GlobalSelectionSystem().setSelectedAll(false);

            // Re-draw the scene
            SceneChangeNotify();
        }
        else {
            gtkutil::MessageBox::ShowError(_("This command is not available in component mode."),
                                 GlobalMainFrame().getTopLevelWindow());
            GlobalRegion().disable();
        }
    }
    else {
        gtkutil::MessageBox::ShowError(
            _("Could not set Region: nothing selected."),
            GlobalMainFrame().getTopLevelWindow());
        GlobalRegion().disable();
    }
}

void RegionManager::traverseRegion(const scene::INodePtr& root, scene::NodeVisitor& nodeExporter)
{
    // Pass the given Walker on to the ExcludeWalker,
    // which calls the nodeExporter.pre() and .post() methods if the visited item is regioned.
    ExcludeNonRegionedWalker visitor(nodeExporter);
    root->traverseChildren(visitor);
}

void RegionManager::saveRegion(const cmd::ArgumentList& args)
{
    // Query the desired filename from the user
    MapFileSelection fileInfo =
        MapFileManager::getMapFileSelection(false, _("Export region"), "region");

    if (!fileInfo.fullPath.empty())
	{
        // Filename is ok, start preparation

        // Save the old region
        AABB oldRegionAABB = GlobalRegion().getRegion();

        // Now check for the effective bounds so that all visible items are included
        AABB visibleBounds = getVisibleBounds();

        // Set the region bounds, but don't traverse the graph!
        GlobalRegion().setRegion(visibleBounds, false);

        // Add the region brushes
        GlobalRegion().addRegionBrushes();

		if (!fileInfo.mapFormat)
		{
			fileInfo.mapFormat = Map::getFormatForFile(fileInfo.fullPath);
		}

        // Save the map and pass the RegionManager::traverseRegion functor
        // that assures that only regioned items are traversed
        MapResource::saveFile(*fileInfo.mapFormat,
                             GlobalSceneGraph().root(),
                             RegionManager::traverseRegion,
                             fileInfo.fullPath);

        // Remove the region brushes
        GlobalRegion().removeRegionBrushes();

        // Set the region AABB back to the state before saving
        GlobalRegion().setRegion(oldRegionAABB, false);

        // Add the filename to the recently used map list
        GlobalMRU().insert(fileInfo.fullPath);
    }
}

void RegionManager::initialiseCommands()
{
    GlobalCommandSystem().addCommand("SaveRegion", saveRegion);
    GlobalCommandSystem().addCommand("RegionOff", disableRegion);
    GlobalCommandSystem().addCommand("RegionSetXY", setRegionXY);
    GlobalCommandSystem().addCommand("RegionSetBrush", setRegionFromBrush);
    GlobalCommandSystem().addCommand("RegionSetSelection", setRegionFromSelection);

    GlobalEventManager().addCommand("SaveRegion", "SaveRegion");
    GlobalEventManager().addCommand("RegionOff", "RegionOff");
    GlobalEventManager().addCommand("RegionSetXY", "RegionSetXY");
    GlobalEventManager().addCommand("RegionSetBrush", "RegionSetBrush");
    GlobalEventManager().addCommand("RegionSetSelection", "RegionSetSelection");
}

} // namespace map

map::RegionManager& GlobalRegion() {
    static map::RegionManagerPtr _regionManager;

    if (_regionManager == NULL) {
        _regionManager = map::RegionManagerPtr(new map::RegionManager);
    }

    return *_regionManager;
}
