#include "EntitySettings.h"

#include "iuimanager.h"
#include "imainframe.h"

#include "registry/registry.h"
#include "registry/adaptors.h"

namespace entity
{

EntitySettings::EntitySettings() :
	_lightVertexColoursLoaded(false)
{
	// Wire up all booleans to update on registry value changes
	initialiseAndObserveKey(RKEY_SHOW_ENTITY_NAMES, _renderEntityNames);
	initialiseAndObserveKey(RKEY_SHOW_ALL_SPEAKER_RADII, _showAllSpeakerRadii);
	initialiseAndObserveKey(RKEY_SHOW_ALL_LIGHT_RADII, _showAllLightRadii);
	initialiseAndObserveKey(RKEY_DRAG_RESIZE_SYMMETRICALLY, _dragResizeEntitiesSymmetrically);
	initialiseAndObserveKey(RKEY_ALWAYS_SHOW_LIGHT_VERTICES, _alwaysShowLightVertices);
	initialiseAndObserveKey(RKEY_FREE_OBJECT_ROTATION, _freeObjectRotation);
	initialiseAndObserveKey(RKEY_SHOW_ENTITY_ANGLES, _showEntityAngles);
}

void EntitySettings::initialiseAndObserveKey(const std::string& key, bool& targetBool)
{
	// Load the initial value from the registry
	targetBool = registry::getValue<bool>(key);

	_registryConnections.emplace_back(registry::observeBooleanKey(key,
		[&]() { targetBool = true; onSettingsChanged(); },
		[&]() { targetBool = false; onSettingsChanged(); })
	);
}

EntitySettingsPtr& EntitySettings::InstancePtr()
{
	static EntitySettingsPtr _entitySettingsInstancePtr(new EntitySettings);

	// Put an assertion here to catch calls after shutdown
	assert(_entitySettingsInstancePtr);

	return _entitySettingsInstancePtr;
}

void EntitySettings::destroy()
{
	// free the instance
	InstancePtr().reset();
}

void EntitySettings::onSettingsChanged()
{
	// Redraw the scene
	GlobalMainFrame().updateAllWindows();
}

void EntitySettings::loadLightVertexColours()
{
	_lightVertexColoursLoaded = true;

	_lightVertexColours[VERTEX_START_END_DESELECTED] = ColourSchemes().getColour("light_startend_deselected");
	_lightVertexColours[VERTEX_START_END_SELECTED] = ColourSchemes().getColour("light_startend_selected");
	_lightVertexColours[VERTEX_INACTIVE] = ColourSchemes().getColour("light_vertex_normal");
	_lightVertexColours[VERTEX_DESELECTED] = ColourSchemes().getColour("light_vertex_deselected");
	_lightVertexColours[VERTEX_SELECTED] = ColourSchemes().getColour("light_vertex_selected");
}

} // namespace entity
