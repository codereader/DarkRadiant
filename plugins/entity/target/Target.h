#ifndef _ENTITY_TARGET_H_
#define _ENTITY_TARGET_H_

#include <boost/shared_ptr.hpp>
#include "scenelib.h"

namespace entity {

/**
 * greebo: This is an abstract representation of a target.
 *         In Doom3 maps, a Target can be any map entity, that's
 *         why this object encapsulates a reference to an actual 
 *         scene::Instance. 
 *
 * Note: Such a Target object can be empty. That's the case for 
 *       entities referring to non-existing entities in their 
 *       "target" spawnarg.
 *
 * All Targets are owned by the TargetManager class.
 *
 * A Target can be referenced by one ore more TargetKey objects.
 */
class Target
{
	// The actual instance this Target refers to (can be NULL)
	scene::Instance* _instance;
public:
	scene::Instance* getInstance() const {
		return _instance;
	}

	void setInstance(scene::Instance* instance) {
		_instance = instance;
	}

	bool isEmpty() const {
		return _instance == NULL;
	}

	void clear() {
		_instance = NULL;
	}

	// greebo: Returns the position of this target or <0,0,0> if empty
	Vector3 getPosition() const {
		if (_instance == NULL) {
			return Vector3(0,0,0);
		}

		return _instance->worldAABB().getOrigin();
	}
};
typedef boost::shared_ptr<Target> TargetPtr;

} // namespace entity

#endif /* _ENTITY_TARGET_H_ */
