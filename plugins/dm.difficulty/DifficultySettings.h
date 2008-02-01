#ifndef DIFFICULTY_SETTINGS_H_
#define DIFFICULTY_SETTINGS_H_

#include "ieclass.h"
#include "entitylib.h"
#include <map>
#include <boost/shared_ptr.hpp>
#include <gtk/gtktreestore.h>

#include "Setting.h"

namespace {
	const std::string RKEY_DIFFICULTY_LEVELS("game/difficulty/numLevels");
	const std::string RKEY_DIFFICULTY_ENTITYDEF_DEFAULT("game/difficulty/defaultSettingsEclass");
	const std::string RKEY_DIFFICULTY_ENTITYDEF_MAP("game/difficulty/mapSettingsEclass");

	enum {
		COL_DESCRIPTION,
		COL_TEXTCOLOUR,
		COL_CLASSNAME,
		COL_SETTING_ID,
		COL_IS_OVERRIDDEN,
		NUM_SETTINGS_COLS,
	};
}

namespace difficulty {

class DifficultySettings
{
	// the difficulty level, these settings are referring to
	int _level;

	// The settings map associates classnames with spawnarg change records.
	// Multiple settings can be made for a single classname.
	typedef std::multimap<std::string, SettingPtr> SettingsMap;
	SettingsMap _settings;

	// A mapping between Setting ID and Setting (for faster lookup on GTK selection)
	typedef std::map<int, SettingPtr> SettingIdMap;
	SettingIdMap _settingIds;

	// This maps classnames to GtkTreeIters, for faster lookup
	typedef std::map<std::string, GtkTreeIter> TreeIterMap;
	TreeIterMap _iterMap;

public:
	// Define the difficulty level in the constructor
	DifficultySettings(int level);

	// Returns the Setting associated with the given className 
	// and carrying the given ID (returned pointer might be NULL)
	SettingPtr getSettingById(int id) const;

	// Returns the level these settings are referring to
	int getLevel() const;

	// Empties the internal structures
	void clear();

	/**
	 * greebo: Saves the given setting. The ID specifices the setting currently 
	 *         highlighted in the editor (pass -1 if nothing is selected).
	 *
	 * @returns: TRUE if successful, FALSE otherwise.
	 */
	bool save(int id, const SettingPtr& setting);

	// Loads the data into the given treestore
	void updateTreeModel(GtkTreeStore* store);

	// Loads all settings (matching the internal _level) from the given entityDef.
	void parseFromEntityDef(const IEntityClassPtr& def);

	// Loads all settings (matching the internal _level) from the given entity.
	void parseFromMapEntity(Entity* entity);

private:
	// Returns true if the given setting is overridden by map-specific settings
	bool isOverridden(const SettingPtr& setting);

	/**
	 * greebo: Returns the TreeIter pointing to the tree element <className> in <store>.
	 * If the item is not yet existing, it gets inserted into the tree, according
	 * to its inheritance tree.
	 */
	GtkTreeIter findOrInsertClassname(GtkTreeStore* store, const std::string& className);

	/**
	 * greebo: Inserts the given classname into the treestore, using the
	 *         given <parent> iter as insertion point. If <parent> is NULL,
	 *         the entry is inserted at root level.
	 */
	GtkTreeIter insertClassName(GtkTreeStore* store, 
							 const std::string& className, 
							 GtkTreeIter* parent = NULL);

	// returns the parent eclass name for the given <className> or "" if no parent
	std::string getParentClass(const std::string& className);
};
typedef boost::shared_ptr<DifficultySettings> DifficultySettingsPtr;

} // namespace difficulty

#endif /* DIFFICULTY_SETTINGS_H_ */
