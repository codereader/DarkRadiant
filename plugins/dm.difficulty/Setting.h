#ifndef __DIFFICULTY_SETTING_H__
#define __DIFFICULTY_SETTING_H__

#include "ieclass.h"

namespace difficulty {

// Smart pointer forward declaration
class Setting;
typedef boost::shared_ptr<Setting> SettingPtr;

/**
 * greebo: A Setting represents a spawnarg change.
 *         This can be an assignment, addition, multiplication or NOP (ignore).
 */
class Setting
{
public:
	enum EApplicationType {
		EAssign,
		EAdd,
		EMultiply,
		EIgnore,
	};

	// The classname this setting applies to
	std::string className;

	// The target spawnarg to be changed
	std::string spawnArg;
	
	// The parsed argument (the specifier (+/*) has already been removed)
	std::string argument;

	// How the argument should be applied
	EApplicationType appType;

	// Whether this setting is valid
	bool isValid;

	// Default constructor
	Setting();

	// Load the settings with the given <index> matching the given <level> from the given dict.
	void parseFromEClass(const IEntityClassPtr& eclass, int level, int index);

	// Factory function: get all Settings from the given eclass (matching the given <level>)
	// The returned list is guaranteed to contain only valid settings.
	//static std::list<SettingPtr> parseSettingsFromEClass(const IEntityClassPtr& eclass, int level);
};

} // namespace difficulty

#endif /* __DIFFICULTY_SETTING_H__ */
