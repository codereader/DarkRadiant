#ifndef _ENTITY_TARGET_H_
#define _ENTITY_TARGET_H_

#include "inode.h"
#include "math/Vector3.h"
#include "math/AABB.h"

namespace entity {

/**
 * greebo: This is an abstract representation of a target.
 * In Doom3 maps, a Target can be any map entity, that's
 * why this object encapsulates a reference to an actual
 * scene::INode.
 *
 * Note: Such a Target object can be empty. That's the case for
 * entities referring to non-existing entities in their
 * "target" spawnarg.
 *
 * All Targets are owned by the TargetManager class.
 *
 * A Target can be referenced by one ore more TargetKey objects.
 */
class Target
{
	// The actual node this Target refers to (can be NULL)
	const scene::INode* _node;

public:
	Target()
	{}

	Target(const scene::INode& node) :
		_node(&node)
	{}

	const scene::INode* getNode() const {
		return _node;
	}

	void setNode(const scene::INode& node) {
		_node = &node;
	}

	bool isEmpty() const {
		return _node == NULL;
	}

	bool isVisible() const {
		return _node != NULL && _node->visible();
	}

	void clear() {
		_node = NULL;
	}

	// greebo: Returns the position of this target or <0,0,0> if empty
	Vector3 getPosition() const
	{
		const scene::INode* node = getNode();

		if (node == NULL) {
			return Vector3(0,0,0);
		}

		return node->worldAABB().getOrigin();
	}
};
typedef boost::shared_ptr<Target> TargetPtr;

} // namespace entity

#endif /* _ENTITY_TARGET_H_ */
