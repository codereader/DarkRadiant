#ifndef _ENTITY_SETTINGS_H_
#define _ENTITY_SETTINGS_H_

#include "iregistry.h"
#include <boost/shared_ptr.hpp>

namespace entity {

namespace {
	const std::string RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
	const std::string RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
	const std::string RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
}

class EntitySettings;
typedef boost::shared_ptr<EntitySettings> EntitySettingsPtr;

/**
 * greebo: A class managing the various settings for entities. It observes
 * the corresponding keyvalues in the registry and updates the internal
 * variables accordingly. This can be used as some sort of "cache" 
 * to avoid slow registry queries during rendering, for instance.
 */
class EntitySettings :
	public RegistryKeyObserver
{
	// TRUE if entity names should be drawn
    bool _renderEntityNames;

	// TRUE if speaker radii should be drawn
	bool _showAllSpeakerRadii;

	// TRUE if light radii should be drawn even when not selected
	bool _showAllLightRadii;

	// Private constructor
	EntitySettings();
public:
	~EntitySettings();

	// RegistryKeyObserver implementation
	void keyChanged(const std::string& key, const std::string& value);

	bool renderEntityNames() {
		return _renderEntityNames;
	}

	bool showAllSpeakerRadii() {
		return _showAllSpeakerRadii;
	}

	bool showAllLightRadii() {
		return _showAllLightRadii;
	}

	// Container for the singleton (ptr)
	static EntitySettingsPtr& InstancePtr();

	// Releases the singleton pointer
	static void destroy();
};

} // namespace entity

#endif /* _ENTITY_SETTINGS_H_ */
