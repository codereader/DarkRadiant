#include "ChildPrimitives.h"

#include "iregistry.h"
#include "ibrush.h"
#include "ientity.h"
#include "igroupnode.h"
#include "scenelib.h"

namespace map
{

// Local helper to add origins
class OriginAdder :
	public scene::NodeVisitor
{
public:
	// NodeVisitor implementation
	bool pre(const scene::INodePtr& node)
	{
		Entity* entity = Node_getEntity(node);

		// Check for an entity
		if (entity != NULL)
		{
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(node);

			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn")
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
	bool pre(const scene::INodePtr& node)
	{
		Entity* entity = Node_getEntity(node);

		// Check for an entity
		if (entity != NULL)
		{
			// greebo: Check for a Doom3Group
			scene::GroupNodePtr groupNode = Node_getGroupNode(node);

			// Don't handle the worldspawn children, they're safe&sound
			if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn")
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
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	OriginAdder adder;
	Node_traverseSubgraph(root, adder);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

void removeOriginFromChildPrimitives(const scene::INodePtr& root)
{
	// Disable texture lock during this process
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	OriginRemover remover;
	Node_traverseSubgraph(root, remover);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

} // namespace
