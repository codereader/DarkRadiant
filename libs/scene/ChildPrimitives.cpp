#include "ChildPrimitives.h"

#include "ibrush.h"
#include "ientity.h"
#include "igroupnode.h"

#include "registry/registry.h"

namespace scene
{

// Local helper to add origins
class OriginAdder :
	public scene::NodeVisitor
{
public:
	// NodeVisitor implementation
	bool pre(const scene::INodePtr& node) override
	{
		Entity* entity = Node_getEntity(node);

		// Check for an entity
		if (entity != nullptr)
		{
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(node);

			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode && !entity->isWorldspawn())
			{
				groupNode->addOriginToChildren();
				// Don't traverse the children
				return false;
			}
		}

		return true;
	}
};

class OriginRemover :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) override
	{
		Entity* entity = Node_getEntity(node);

		// Check for an entity
		if (entity != nullptr)
		{
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(node);

			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode && !entity->isWorldspawn())
			{
				groupNode->removeOriginFromChildren();
				// Don't traverse the children
				return false;
			}
		}

		return true;
	}
};

void addOriginToChildPrimitives(const scene::INodePtr& root)
{
	// Disable texture lock during this process
    registry::ScopedKeyChanger<bool> changer(RKEY_ENABLE_TEXTURE_LOCK, false);

	OriginAdder adder;
	root->traverse(adder);
}

void removeOriginFromChildPrimitives(const scene::INodePtr& root)
{
	// Disable texture lock during this process
    registry::ScopedKeyChanger<bool> changer(RKEY_ENABLE_TEXTURE_LOCK, false);

	OriginRemover remover;
	root->traverse(remover);
}

} // namespace
