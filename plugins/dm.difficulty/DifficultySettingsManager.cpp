#include "DifficultySettingsManager.h"

#include "ieclass.h"
#include "iregistry.h"
#include "stream/textstream.h"

namespace difficulty {

	namespace {
		const std::string RKEY_DIFFICULTY_LEVELS("game/difficulty/numLevels");
		const std::string RKEY_DIFFICULTY_ENTITYDEF_DEFAULT("game/difficulty/defaultSettingsEclass");
		const std::string RKEY_DIFFICULTY_ENTITYDEF_MAP("game/difficulty/mapSettingsEclass");
	}

void DifficultySettingsManager::loadSettings() {
	loadDefaultSettings();
	loadMapSettings();
}

void DifficultySettingsManager::loadDefaultSettings() {
	// Try to lookup the given entityDef
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(RKEY_DIFFICULTY_ENTITYDEF_DEFAULT);

	if (eclass == NULL) {
		globalErrorStream() << "Could not find default difficulty settings entityDef.\n";
		return;
	}

	// greebo: Setup the default difficulty levels using the found entityDef
	int numLevels = GlobalRegistry().getInt(RKEY_DIFFICULTY_LEVELS);
	for (int i = 0; i < numLevels; i++) {
		// Allocate a new settings object
		DifficultySettingsPtr settings(new DifficultySettings(i));

		// Parse the settings from the given default settings entityDef
		settings->parseFromEntityDef(eclass);

		// Store the settings object in the local list
		_settings.push_back(settings);
	}
}

void DifficultySettingsManager::loadMapSettings() {
	// TODO
}

} // namespace difficulty
