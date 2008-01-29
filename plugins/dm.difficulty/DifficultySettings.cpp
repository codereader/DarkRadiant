#include "DifficultySettings.h"

#include "SettingsLoaderEClass.h"

namespace difficulty {

DifficultySettings::DifficultySettings(int level) :
	_level(level)
{}

int DifficultySettings::getLevel() const {
	return _level;
}

void DifficultySettings::clear() {
	_settings.clear();
}

void DifficultySettings::parseFromEntityDef(const IEntityClassPtr& def) {
	// Instantiate a helper class and traverse the eclass
	SettingsLoaderEClass visitor(def, *this);
	def->forEachClassAttribute(visitor);
}

} // namespace difficulty
