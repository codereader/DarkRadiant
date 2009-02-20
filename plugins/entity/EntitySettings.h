#ifndef _ENTITY_SETTINGS_H_
#define _ENTITY_SETTINGS_H_

#include "iregistry.h"
#include <boost/shared_ptr.hpp>

namespace entity {

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

	// Private constructor
	EntitySettings();
public:
	~EntitySettings();

	// RegistryKeyObserver implementation
	void keyChanged(const std::string& key, const std::string& value);

	bool renderEntityNames() {
		return _renderEntityNames;
	}

	// Container for the singleton (ptr)
	static EntitySettingsPtr& InstancePtr();

	// Releases the singleton pointer
	static void destroy();
};

} // namespace entity

#endif /* _ENTITY_SETTINGS_H_ */
