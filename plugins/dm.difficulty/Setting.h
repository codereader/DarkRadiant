#ifndef __DIFFICULTY_SETTING_H__
#define __DIFFICULTY_SETTING_H__

#include <string>
#include <boost/shared_ptr.hpp>

typedef struct _GtkListStore GtkListStore;

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
		ENumAppTypes,
	};

	// ID of this setting (unique for each difficulty level)
	int id;

	// The classname this setting applies to
	std::string className;

	// The target spawnarg to be changed
	std::string spawnArg;
	
	// The parsed argument (the specifier (+/*) has already been removed)
	std::string argument;

	// How the argument should be applied
	EApplicationType appType;

	// Whether this setting is a default setting or map-specific
	bool isDefault;

	// Constructor (assigns a unique ID automatically)
	Setting();

	// Comparison operators, return true if spawnarg/classname/appType are the same
	bool operator==(const Setting& rhs) const;
	bool operator!=(const Setting& rhs) const;

	// Assignment operator (leaves the ID untouched)
	Setting& operator=(const Setting& rhs);

	// Interprets the argument string
	void parseAppType();

	// Get the string suitable for saving to a spawnarg (e.g. "+50")
	std::string getArgumentKeyValue() const;

	// Assemble a description string for the contained spawnArg/argument combo.
	std::string getDescString() const;

	// Creates a new GtkListStore instance for packing into a treeview
	static GtkListStore* getAppTypeStore();

private:
	static int _highestId;
};
typedef boost::shared_ptr<Setting> SettingPtr;

} // namespace difficulty

#endif /* __DIFFICULTY_SETTING_H__ */
