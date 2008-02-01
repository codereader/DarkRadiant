#ifndef DIFFICULTY_ENTITY_H_
#define DIFFICULTY_ENTITY_H_

#include "ientity.h"
#include "Setting.h"

namespace difficulty {

/**
 * greebo: A DifficultyEntity encapsulates a doom3-compatible map entity
 *         and provides methods to save/load difficulty settings to/from spawargs.
 */
class DifficultyEntity
{
	// The actual entity we're working with
	Entity* _entity;

	// The unique ID needed to write the spawnargs
	int _curId;

public:
	DifficultyEntity(Entity* source);

	// Removes all difficulty settings from the entity
	void clear();

	// Write the setting to this entity
	void writeSetting(const SettingPtr& setting, int _level);
};
typedef boost::shared_ptr<DifficultyEntity> DifficultyEntityPtr;

} // namespace difficulty

#endif /* DIFFICULTY_ENTITY_H_ */
