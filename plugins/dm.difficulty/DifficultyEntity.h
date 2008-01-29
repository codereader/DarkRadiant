#ifndef DIFFICULTY_ENTITY_H_
#define DIFFICULTY_ENTITY_H_

#include "ientity.h"

namespace difficulty {

/**
 * greebo: A DifficultyEntity encapsulates a doom3-compatible map entity
 *         and provides methods to save/load difficulty settings to/from spawargs.
 */
class DifficultyEntity
{
	// The actual entity we're working with
	Entity* _entity;

public:
	DifficultyEntity(Entity* source);
};
typedef boost::shared_ptr<DifficultyEntity> DifficultyEntityPtr;

} // namespace difficulty

#endif /* DIFFICULTY_ENTITY_H_ */
