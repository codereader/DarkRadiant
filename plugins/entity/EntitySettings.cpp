#include "EntitySettings.h"

#include "iuimanager.h"
#include "imainframe.h"

#include "registry/registry.h"

namespace entity {

EntitySettings::EntitySettings() :
	_renderEntityNames(registry::getValue<bool>(RKEY_SHOW_ENTITY_NAMES)),
	_showAllSpeakerRadii(registry::getValue<bool>(RKEY_SHOW_ALL_SPEAKER_RADII)),
	_showAllLightRadii(registry::getValue<bool>(RKEY_SHOW_ALL_LIGHT_RADII)),
	_dragResizeEntitiesSymmetrically(registry::getValue<bool>(RKEY_DRAG_RESIZE_SYMMETRICALLY)),
	_alwaysShowLightVertices(registry::getValue<bool>(RKEY_ALWAYS_SHOW_LIGHT_VERTICES)),
	_freeModelRotation(registry::getValue<bool>(RKEY_FREE_MODEL_ROTATION)),
	_showEntityAngles(registry::getValue<bool>(RKEY_SHOW_ENTITY_ANGLES)),
	_lightVertexColoursLoaded(false)
{
	// Register this class as keyobserver
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_NAMES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_SPEAKER_RADII);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_LIGHT_RADII);
	GlobalRegistry().addKeyObserver(this, RKEY_DRAG_RESIZE_SYMMETRICALLY);
	GlobalRegistry().addKeyObserver(this, RKEY_ALWAYS_SHOW_LIGHT_VERTICES);
	GlobalRegistry().addKeyObserver(this, RKEY_FREE_MODEL_ROTATION);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_ANGLES);
}

EntitySettings::~EntitySettings() 
{
	GlobalRegistry().removeKeyObserver(this);
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

// RegistryKeyObserver implementation
void EntitySettings::keyChanged(const std::string& key, const std::string& value)
{
	// Update the internal value
	if (key == RKEY_SHOW_ENTITY_NAMES)
	{
        _renderEntityNames = (value == "1");
	}
	else if (key == RKEY_SHOW_ALL_SPEAKER_RADII)
	{
		_showAllSpeakerRadii = (value == "1");
	}
	else if (key == RKEY_SHOW_ALL_LIGHT_RADII)
	{
		_showAllLightRadii = (value == "1");
	}
	else if (key == RKEY_DRAG_RESIZE_SYMMETRICALLY)
	{
		_dragResizeEntitiesSymmetrically = (value == "1");
	}
	else if (key == RKEY_ALWAYS_SHOW_LIGHT_VERTICES)
	{
		_alwaysShowLightVertices = (value == "1");
	}
	else if (key == RKEY_FREE_MODEL_ROTATION)
	{
		_freeModelRotation = (value == "1");
	}
	else if (key == RKEY_SHOW_ENTITY_ANGLES)
	{
		_showEntityAngles = (value == "1");
	}

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
