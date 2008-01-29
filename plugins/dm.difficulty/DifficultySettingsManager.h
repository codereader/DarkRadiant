#ifndef DIFFICULTY_SETTINGS_MANAGER_H_
#define DIFFICULTY_SETTINGS_MANAGER_H_

#include "DifficultySettings.h"

namespace {
	const std::string RKEY_DIFFICULTY_LEVELS("game/difficulty/numLevels");
	const std::string RKEY_DIFFICULTY_ENTITYDEF_DEFAULT("game/difficulty/defaultSettingsEclass");
	const std::string RKEY_DIFFICULTY_ENTITYDEF_MAP("game/difficulty/mapSettingsEclass");
}

namespace difficulty {

/**
 * greebo: The DifficultyManager contains all settings of all
 *         available difficulty levels. The manager also provides
 *         methods to fill in the settings data into a given
 *         GtkTreeModel and to save the existing settings to the map/entityDef.
 **/
class DifficultySettingsManager
{
	// This contains all the settings of all the difficulty levels
	std::vector<DifficultySettingsPtr> _settings;

public:
	// Loads all settings from the entityDefs and the currently loaded map.
	void loadSettings();

	// Get the settings object for the given difficulty <level>
	DifficultySettingsPtr getSettings(int level);

private:
	void loadDefaultSettings();
	void loadMapSettings();
};

} // namespace difficulty

#endif /* DIFFICULTY_SETTINGS_MANAGER_H_ */
