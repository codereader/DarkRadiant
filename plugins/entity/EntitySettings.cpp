#include "EntitySettings.h"

#include "imainframe.h"

namespace entity {

EntitySettings::EntitySettings() :
	_renderEntityNames(GlobalRegistry().get(RKEY_SHOW_ENTITY_NAMES) == "1"),
	_showAllSpeakerRadii(GlobalRegistry().get(RKEY_SHOW_ALL_SPEAKER_RADII) == "1"),
	_showAllLightRadii(GlobalRegistry().get(RKEY_SHOW_ALL_LIGHT_RADII) == "1"),
	_dragResizeEntitiesSymmetrically(GlobalRegistry().get(RKEY_DRAG_RESIZE_SYMMETRICALLY) == "1"),
	_alwaysShowLightVertices(GlobalRegistry().get(RKEY_ALWAYS_SHOW_LIGHT_VERTICES) == "1")
{
	// Register this class as keyobserver
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_NAMES);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_SPEAKER_RADII);
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ALL_LIGHT_RADII);
	GlobalRegistry().addKeyObserver(this, RKEY_DRAG_RESIZE_SYMMETRICALLY);
	GlobalRegistry().addKeyObserver(this, RKEY_ALWAYS_SHOW_LIGHT_VERTICES);
}

EntitySettings::~EntitySettings() {
	GlobalRegistry().removeKeyObserver(this);
}

EntitySettingsPtr& EntitySettings::InstancePtr() {
	static EntitySettingsPtr _entitySettingsInstancePtr(new EntitySettings);

	// Put an assertion here to catch calls after shutdown
	assert(_entitySettingsInstancePtr != NULL);

	return _entitySettingsInstancePtr;
}

void EntitySettings::destroy() {
	// free the instance
	InstancePtr() = EntitySettingsPtr();
}

// RegistryKeyObserver implementation
void EntitySettings::keyChanged(const std::string& key, const std::string& value)
{
	// Update the internal value
	if (key == RKEY_SHOW_ENTITY_NAMES) {
        _renderEntityNames = (value == "1");
	}
	else if (key == RKEY_SHOW_ALL_SPEAKER_RADII) {
		_showAllSpeakerRadii = (value == "1");
	}
	else if (key == RKEY_SHOW_ALL_LIGHT_RADII) {
		_showAllLightRadii = (value == "1");
	}
	else if (key == RKEY_DRAG_RESIZE_SYMMETRICALLY) {
		_dragResizeEntitiesSymmetrically = (value == "1");
	}
	else if (key == RKEY_ALWAYS_SHOW_LIGHT_VERTICES)
	{
		_alwaysShowLightVertices = (value == "1");
	}

	// Redraw the scene
	GlobalMainFrame().updateAllWindows();
}

} // namespace entity
