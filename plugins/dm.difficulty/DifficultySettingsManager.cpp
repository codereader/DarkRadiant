#include "DifficultySettingsManager.h"

#include "ieclass.h"
#include "iregistry.h"
#include "stream/textstream.h"
#include "DifficultyEntityFinder.h"

namespace difficulty {

DifficultySettingsPtr DifficultySettingsManager::getSettings(int level) {
	for (std::size_t i = 0; i < _settings.size(); i++) {
		if (_settings[i]->getLevel() == level) {
			return _settings[i];
		}
	}
	return DifficultySettingsPtr();
}

void DifficultySettingsManager::loadSettings() {
	loadDefaultSettings();
	loadMapSettings();
}

void DifficultySettingsManager::loadDefaultSettings() {
	// Try to lookup the given entityDef
	IEntityClassPtr eclass = GlobalEntityClassManager().findClass(
		GlobalRegistry().get(RKEY_DIFFICULTY_ENTITYDEF_DEFAULT)
	);

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
	// Construct a helper walker
	DifficultyEntityFinder finder;
	GlobalSceneGraph().traverse(finder);

	const DifficultyEntityFinder::EntityList& found = finder.getEntities();

	// Pop all entities into each difficulty setting
	for (DifficultyEntityFinder::EntityList::const_iterator ent = found.begin(); 
		 ent != found.end(); ent++)
	{
		for (std::size_t i = 0; i < _settings.size(); i++) {
			_settings[i]->parseFromMapEntity(*ent);
		}
	}
}

} // namespace difficulty
