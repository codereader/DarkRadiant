#ifndef SRPROPERTYSAVER_H_
#define SRPROPERTYSAVER_H_

#include "StimResponse.h"
#include "SREntity.h"

// Forward declaration
class Entity;

/** greebo: Visitor class - saves the spawnargs to the given entity.
 */
class SRPropertySaver
{
	Entity* _target;
	
	// The list of allowed keys
	SREntity::KeyList& _keys;

public:
	SRPropertySaver(Entity* target, SREntity::KeyList& keys);

	/** greebo: Visitor method, saves the spawnargs to the 
	 * 			target entity.
	 */
	void visit(StimResponse& sr);
};

#endif /*SRPROPERTYSAVER_H_*/
