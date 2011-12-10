#include "EntitySettings.h"

#include "iuimanager.h"
#include "imainframe.h"

#include "registry/registry.h"

namespace entity {

EntitySettings::EntitySettings() :
	_lightVertexColoursLoaded(false)
{
    refreshFromRegistry();

	// Register this class as keyobserver
	observeKey(RKEY_SHOW_ENTITY_NAMES);
	observeKey(RKEY_SHOW_ALL_SPEAKER_RADII);
	observeKey(RKEY_SHOW_ALL_LIGHT_RADII);
	observeKey(RKEY_DRAG_RESIZE_SYMMETRICALLY);
	observeKey(RKEY_ALWAYS_SHOW_LIGHT_VERTICES);
	observeKey(RKEY_FREE_MODEL_ROTATION);
	observeKey(RKEY_SHOW_ENTITY_ANGLES);
}

void EntitySettings::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &EntitySettings::keyChanged)
    );
}

EntitySettingsPtr& EntitySettings::InstancePtr()
{
	static EntitySettingsPtr _entitySettingsInstancePtr(new EntitySettings);

	// Put an assertion here to catch calls after shutdown
	assert(_entitySettingsInstancePtr != NULL);

	return _entitySettingsInstancePtr;
}

void EntitySettings::destroy()
{
	// free the instance
	InstancePtr() = EntitySettingsPtr();
}

void EntitySettings::refreshFromRegistry()
{
    _renderEntityNames = registry::getValue<bool>(RKEY_SHOW_ENTITY_NAMES);
    _showAllSpeakerRadii = registry::getValue<bool>(RKEY_SHOW_ALL_SPEAKER_RADII);
    _showAllLightRadii = registry::getValue<bool>(RKEY_SHOW_ALL_LIGHT_RADII);
    _dragResizeEntitiesSymmetrically = registry::getValue<bool>(RKEY_DRAG_RESIZE_SYMMETRICALLY);
    _alwaysShowLightVertices = registry::getValue<bool>(RKEY_ALWAYS_SHOW_LIGHT_VERTICES);
    _freeModelRotation = registry::getValue<bool>(RKEY_FREE_MODEL_ROTATION);
    _showEntityAngles = registry::getValue<bool>(RKEY_SHOW_ENTITY_ANGLES);
}

void EntitySettings::keyChanged()
{
    refreshFromRegistry();

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
