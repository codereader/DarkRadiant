#include "EntitySettings.h"

namespace entity {

namespace {
	const std::string RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
}

EntitySettings::EntitySettings() :
	_renderEntityNames(GlobalRegistry().get(RKEY_SHOW_ENTITY_NAMES) == "1")
{
	// Register this class as keyobserver
	GlobalRegistry().addKeyObserver(this, RKEY_SHOW_ENTITY_NAMES);
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
void EntitySettings::keyChanged(const std::string& key, const std::string& value) {
	// Update the internal value
	if (key == RKEY_SHOW_ENTITY_NAMES) {
        _renderEntityNames = (value == "1");
	}
}

} // namespace entity
