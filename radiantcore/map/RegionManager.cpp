#include "RegionManager.h"

#include "i18n.h"
#include "ibrush.h"
#include "icameraview.h"
#include "iorthoview.h"
#include "ientity.h"
#include "ieclass.h"
#include "imru.h"
#include "ifiletypes.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "imapformat.h"

#include "scenelib.h"
#include "selectionlib.h"
#include "shaderlib.h"
#include "gamelib.h"
#include "string/string.h"

#include "registry/registry.h"
#include "brush/Brush.h"
#include "RegionWalkers.h"
#include "MapFileManager.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/General.h"
#include "map/MapResource.h"
#include "map/Map.h"
#include "module/StaticModule.h"
#include "command/ExecutionFailure.h"
#include "command/ExecutionNotPossible.h"

#include <memory>

namespace map
{

namespace
{
    typedef std::shared_ptr<RegionManager> RegionManagerPtr;
    const std::string GKEY_PLAYER_START_ECLASS = "/mapFormat/playerStartPoint";
}

RegionManager::RegionManager() :
    _active(false)
{}

bool RegionManager::isEnabled() const
{
    return _active;
}

void RegionManager::disable()
{
    _active = false;

    _bounds = AABB::createFromMinMax(Vector3(1,1,1)*_worldMin, Vector3(1,1,1)*_worldMax);
    // Create a 64 unit border from the max world coord
    _bounds.extents -= Vector3(1,1,1)*64;

    if (GlobalSceneGraph().root())
    {
        // Disable the exclude bit on all the scenegraph nodes
        ExcludeAllWalker walker(false);
        GlobalSceneGraph().root()->traverse(walker);
    }
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

AABB RegionManager::getRegionBounds()
{
    if (isEnabled())
    {
        return _bounds;
    }

    // Set the region bounds to the maximum available size
    return AABB(Vector3(0, 0, 0), Vector3(_worldMax, _worldMax, _worldMax));
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
    for (int i = 0; i < 6; i++)
    {
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
    auto eClassPlayerStart = game::current::getValue<std::string>(GKEY_PLAYER_START_ECLASS);
    auto playerStart = GlobalEntityClassManager().findOrInsert(eClassPlayerStart, false);

    // Create the info_player_start entity
    _playerStart = GlobalEntityModule().createEntity(playerStart);

    try
    {
        auto& camView = GlobalCameraManager().getActiveView();

        // Obtain the camera origin = player start point
        Vector3 camOrigin = camView.getCameraOrigin();
        // Get the start angle of the player start point
        auto angle = camView.getCameraAngles()[camera::CAMERA_YAW];

        // Check if the camera origin is within the region
        if (!_bounds.intersects(camOrigin))
        {
            throw cmd::ExecutionFailure(
                _("Warning: Camera not within region, can't set info_player_start.")
            );
        }

        // Set the origin key of the playerStart entity
        _playerStart->getEntity().setKeyValue("origin", string::to_string(camOrigin));
        _playerStart->getEntity().setKeyValue("angle", string::to_string(angle));
    }
    catch (const cmd::ExecutionFailure& ex)
    {
        throw ex; // let this one slip through
    }
    catch (const std::runtime_error&)
    {
        // CamWnd not available, log this
        throw cmd::ExecutionFailure("Failed to read camera position.");
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
      							texdef_name_default());
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
      							texdef_name_default());
		}
	}
}

void RegionManager::removeRegionBrushes() {
    for (int i = 0; i < 6; i++) {
        // Remove the brushes from the scene
        if (_brushes[i] != NULL) {
            GlobalMap().findOrInsertWorldspawn()->removeChildNode(_brushes[i]);
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
    disable();
    SceneChangeNotify();
}

void RegionManager::setRegionXY(const cmd::ArgumentList& args)
{
    try
    {
        if (!module::GlobalModuleRegistry().moduleExists(MODULE_ORTHOVIEWMANAGER))
        {
            throw std::runtime_error("No ortho view module loaded.");
        }

        // Obtain the current XY orthoview, if there is one
        auto& xyWnd = GlobalXYWndManager().getViewByType(XY);
        const auto& origin = xyWnd.getOrigin();

        Vector2 topLeft(
            origin[0] - 0.5f * xyWnd.getWidth() / xyWnd.getScale(),
            origin[1] - 0.5f * xyWnd.getHeight() / xyWnd.getScale()
        );

        Vector2 lowerRight(
            origin[0] + 0.5f * xyWnd.getWidth() / xyWnd.getScale(),
            origin[1] + 0.5f * xyWnd.getHeight() / xyWnd.getScale()
        );

        // Set the bounds from the calculated XY rectangle
        setRegionFromXY(topLeft, lowerRight);

        SceneChangeNotify();
    }
    catch (const std::runtime_error&)
    {
        disable();
        throw cmd::ExecutionFailure(_("Could not set Region: XY Top View not found."));
    }
}

void RegionManager::setRegionFromBrush(const cmd::ArgumentList& args)
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // Check, if exactly one brush is selected
    if (info.brushCount == 1 && info.totalCount == 1) {
        // Get the selected node
        const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();

        // Set the bounds of the region to the selection's extents
        setRegion(node->worldAABB());

        // Delete the currently selected brush (undoable command)
        {
            UndoableCommand undo("deleteSelected");
            selection::algorithm::deleteSelection();
        }

        SceneChangeNotify();
    }
    else
    {
        disable();
        throw cmd::ExecutionFailure(_("Could not set Region: please select a single Brush."));
    }
}

void RegionManager::setRegionFromSelection(const cmd::ArgumentList& args)
{
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    // Check, if there is anything selected
    if (info.totalCount > 0)
    {
        if (GlobalSelectionSystem().Mode() != selection::SelectionSystem::eComponent)
        {
            // Obtain the selection size (its min/max vectors)
            AABB regionBounds = GlobalSelectionSystem().getWorkZone().bounds;

            // Set the region
            setRegion(regionBounds);

            // De-select all the selected items
            GlobalSelectionSystem().setSelectedAll(false);

            // Re-draw the scene
            SceneChangeNotify();
        }
        else
        {
            disable();
            throw cmd::ExecutionNotPossible(_("This command is not available in component mode."));
        }
    }
    else
    {
        disable();
        throw cmd::ExecutionNotPossible(_("Cannot set Region: nothing selected."));
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
        MapFileManager::getMapFileSelection(false, _("Export region"), filetype::TYPE_REGION);

    if (!fileInfo.fullPath.empty())
	{
        // Filename is ok, start preparation

        // Save the old region
        AABB oldRegionAABB = getRegion();

        // Now check for the effective bounds so that all visible items are included
        AABB visibleBounds = getVisibleBounds();

        // Set the region bounds, but don't traverse the graph!
        setRegion(visibleBounds, false);

        // Add the region brushes
        addRegionBrushes();

		if (!fileInfo.mapFormat)
		{
			fileInfo.mapFormat = GlobalMap().getMapFormatForFilenameSafe(fileInfo.fullPath);
		}

        // Save the map and pass the RegionManager::traverseRegion functor
        // that assures that only regioned items are traversed
        MapResource::saveFile(*fileInfo.mapFormat,
                             GlobalSceneGraph().root(),
                             RegionManager::traverseRegion,
                             fileInfo.fullPath);

        // Remove the region brushes
        removeRegionBrushes();

        // Set the region AABB back to the state before saving
        setRegion(oldRegionAABB, false);

        // Add the filename to the recently used map list
        GlobalMRU().insert(fileInfo.fullPath);
    }
}

void RegionManager::initialiseCommands()
{
    GlobalCommandSystem().addCommand("SaveRegion", std::bind(&RegionManager::saveRegion, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("RegionOff", std::bind(&RegionManager::disableRegion, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("RegionSetXY", std::bind(&RegionManager::setRegionXY, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("RegionSetBrush", std::bind(&RegionManager::setRegionFromBrush, this, std::placeholders::_1));
    GlobalCommandSystem().addCommand("RegionSetSelection", std::bind(&RegionManager::setRegionFromSelection, this, std::placeholders::_1));
}

const std::string& RegionManager::getName() const
{
	static std::string _name(MODULE_REGION_MANAGER);
	return _name;
}

const StringSet& RegionManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void RegionManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	initialiseCommands();

	_worldMin = game::current::getValue<float>("/defaults/minWorldCoord");
	_worldMax = game::current::getValue<float>("/defaults/maxWorldCoord");

	for (int i = 0; i < 6; i++)
	{
		_brushes[i].reset();
	}

	GlobalMap().signal_mapEvent().connect(
		sigc::mem_fun(*this, &RegionManager::onMapEvent));
}

void RegionManager::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloading)
	{
		// Turn regioning off when unloading the map
		disable();
		clear();
	}
	else if (ev == IMap::MapLoaded)
	{
		// Disable when a new map has been loaded
		disable();
	}
}

AABB RegionManager::getVisibleBounds()
{
	AABB returnValue;

	GlobalSceneGraph().root()->foreachNode([&](const scene::INodePtr& node)
	{
		if (node->visible())
		{
			returnValue.includeAABB(node->worldAABB());
		}

		return true;
	});

	return returnValue;
}

module::StaticModuleRegistration<RegionManager> staticRegionManagerModule;

} // namespace
