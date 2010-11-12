#include "InstanceWalkers.h"

#include "iscenegraph.h"

namespace scene {

bool InstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	// greebo: Register this new node with the scenegraph
	if (!node->inScene())
	{
		GlobalSceneGraph().insert(node);
	}

	_nodeStack.push(node);

	return true;
}

void InstanceSubgraphWalker::post(const INodePtr& node)
{
	_nodeStack.pop();

	if (!_nodeStack.empty())
	{
		if (node->getParent() != _nodeStack.top())
		{
			// Parent-child mismatch, adjust
			node->setParent(_nodeStack.top());
		}
	}
}

// ==============================================================================================

bool UninstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	return true;
}

void UninstanceSubgraphWalker::post(const scene::INodePtr& node)
{
	// Notify the Scenegraph about the upcoming deletion
	if (node->inScene())
	{
		GlobalSceneGraph().erase(node);
	}
}

} // namespace scene
