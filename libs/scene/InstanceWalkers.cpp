#include "InstanceWalkers.h"

#include "iscenegraph.h"

namespace scene {

bool InstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	// greebo: Register this new node with the scenegraph 
	GlobalSceneGraph().insert(node);

	return true;
}

// ==============================================================================================

bool UninstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	return true;
}

void UninstanceSubgraphWalker::post(const scene::INodePtr& node)
{
	// Notify the Scenegraph about the upcoming deletion
	GlobalSceneGraph().erase(node);
}

} // namespace scene
