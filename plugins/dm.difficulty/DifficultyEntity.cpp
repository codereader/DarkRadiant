#include "DifficultyEntity.h"

#include "ientity.h"

namespace difficulty {

DifficultyEntity::DifficultyEntity(Entity* source) :
	_entity(source)
{}

void DifficultyEntity::clear() {
	// Find all spawnargs starting with a "diff_"
	Entity::KeyValuePairs pairs = _entity->getKeyValuePairs("diff_");

	// Remove all (set the value to the empty string "")
	for (Entity::KeyValuePairs::iterator i = pairs.begin(); i != pairs.end(); i++) {
		_entity->setKeyValue(i->first, "");
	}
}

} // namespace difficulty
