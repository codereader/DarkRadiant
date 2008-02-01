#include "DifficultyEntity.h"

#include "ientity.h"
#include "string/string.h"

namespace difficulty {

DifficultyEntity::DifficultyEntity(Entity* source) :
	_entity(source),
	_curId(0)
{}

void DifficultyEntity::clear() {
	// Find all spawnargs starting with a "diff_"
	Entity::KeyValuePairs pairs = _entity->getKeyValuePairs("diff_");

	// Remove all (set the value to the empty string "")
	for (Entity::KeyValuePairs::iterator i = pairs.begin(); i != pairs.end(); i++) {
		_entity->setKeyValue(i->first, "");
	}

	_curId = 0;
}

void DifficultyEntity::writeSetting(const SettingPtr& setting, int _level) {
	// Construct the prefix and index strings
	std::string prefix = "diff_" + intToStr(_level) + "_";
	std::string idx = intToStr(_curId);

	// Save the spawnargs
	_entity->setKeyValue(prefix + "class_" + idx, setting->className);
	_entity->setKeyValue(prefix + "change_" + idx, setting->spawnArg);
	_entity->setKeyValue(prefix + "arg_" + idx, setting->getArgumentKeyValue());

	// Increase the ID
	_curId++;
}

} // namespace difficulty
