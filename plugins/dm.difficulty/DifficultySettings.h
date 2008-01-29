#ifndef DIFFICULTY_SETTINGS_H_
#define DIFFICULTY_SETTINGS_H_

#include "ieclass.h"
#include <map>
#include <boost/shared_ptr.hpp>

#include "Setting.h"

typedef struct _GtkTreeStore GtkTreeStore;

namespace difficulty {

class DifficultySettings
{
	// the difficulty level, these settings are referring to
	int _level;

	// The settings map associates classnames with spawnarg change records.
	// Multiple settings can be made for a single classname.
	typedef std::multimap<std::string, SettingPtr> SettingsMap;
	SettingsMap _settings;

public:
	// Define the difficulty level in the constructor
	DifficultySettings(int level);

	// Returns the level these settings are referring to
	int getLevel() const;

	// Empties the internal structures
	void clear();

	// Loads the data into the given treestore
	void updateTreeModel(GtkTreeStore* store);

	// Loads all settings (matching the internal _level) from the given entityDef.
	void parseFromEntityDef(const IEntityClassPtr& def);
};
typedef boost::shared_ptr<DifficultySettings> DifficultySettingsPtr;

} // namespace difficulty

#endif /* DIFFICULTY_SETTINGS_H_ */
