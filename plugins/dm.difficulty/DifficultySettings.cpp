#include "DifficultySettings.h"

namespace difficulty {

DifficultySettings::DifficultySettings(int level) :
	_level(level)
{}

int DifficultySettings::getLevel() const {
	return _level;
}

void DifficultySettings::clear() {

}

void DifficultySettings::parseFromEntityDef(const IEntityClassPtr& def) {
	// TODO (copy from darkmod_src)
}

} // namespace difficulty
