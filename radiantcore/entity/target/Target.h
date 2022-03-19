#pragma once

#include "inode.h"
#include "ientity.h"
#include "ilightnode.h"
#include "math/Vector3.h"
#include "math/AABB.h"
#include <sigc++/signal.h>

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
class Target :
    public ITargetableObject
{
	// The actual node this Target refers to (can be NULL)
	const scene::INode* _node;

    sigc::signal<void> _sigPositionChanged;

public:
	Target() :
        _node(nullptr)
	{}

	Target(const scene::INode& node) :
		_node(&node)
	{}

	const scene::INode* getNode() const override
    {
		return _node;
	}

	void setNode(const scene::INode& node)
    {
		_node = &node;
	}

	bool isEmpty() const override
    {
		return _node == nullptr;
	}

	bool isVisible() const
    {
        return _node != nullptr && _node->visible();
	}

	void clear()
    {
        // Notify any watchers on scene removal
        signal_TargetChanged().emit();
        _node = nullptr;
	}

	// greebo: Returns the position of this target or <0,0,0> if empty
	Vector3 getPosition() const
	{
		const scene::INode* node = getNode();

        if (node == nullptr)
        {
			return Vector3(0,0,0);
		}

		// Check if we're targeting a light, and use its center in that case (#5151)
		auto lightNode = dynamic_cast<const ILightNode*>(node);

		if (lightNode != nullptr)
		{
			return lightNode->getSelectAABB().getOrigin();
		}
		
		return node->worldAABB().getOrigin();
	}

    // Invoked by the TargetManager when an entity's position changed
    void onPositionChanged()
	{
        signal_TargetChanged().emit();
	}

    // Invoked by the TargetManager when an entity's visibility has changed
    void onVisibilityChanged()
	{
        signal_TargetChanged().emit();
	}

    sigc::signal<void>& signal_TargetChanged()
	{
        return _sigPositionChanged;
	}
};
typedef std::shared_ptr<Target> TargetPtr;

} // namespace entity
