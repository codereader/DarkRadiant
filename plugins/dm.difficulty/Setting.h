#ifndef __DIFFICULTY_SETTING_H__
#define __DIFFICULTY_SETTING_H__

#include <string>
#include <boost/shared_ptr.hpp>

namespace difficulty {

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

	// Interprets the argument string
	void parseAppType();

	// Assemble a description string for the contained spawnArg/argument combo.
	std::string getDescString() const;
};
typedef boost::shared_ptr<Setting> SettingPtr;

} // namespace difficulty

#endif /* __DIFFICULTY_SETTING_H__ */
