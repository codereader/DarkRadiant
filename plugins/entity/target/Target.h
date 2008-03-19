#ifndef _ENTITY_TARGET_H_
#define _ENTITY_TARGET_H_

#include "inode.h"
#include "math/Vector3.h"
#include "math/aabb.h"

namespace entity {

/**
 * greebo: This is an abstract representation of a target.
 *         In Doom3 maps, a Target can be any map entity, that's
 *         why this object encapsulates a reference to an actual 
 *         scene::INodePtr. 
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
	// The actual node this Target refers to (can be NULL)
	scene::INodeWeakPtr _node;
public:
	scene::INodePtr getNode() const {
		return _node.lock();
	}

	void setNode(const scene::INodePtr& node) {
		_node = node;
	}

	bool isEmpty() const {
		return getNode() == NULL;
	}

	void clear() {
		_node = scene::INodePtr();
	}

	// greebo: Returns the position of this target or <0,0,0> if empty
	Vector3 getPosition() const {
		scene::INodePtr node = getNode();

		if (node == NULL) {
			return Vector3(0,0,0);
		}

		return node->worldAABB().getOrigin();
	}
};
typedef boost::shared_ptr<Target> TargetPtr;

} // namespace entity

#endif /* _ENTITY_TARGET_H_ */
