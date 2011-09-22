#include "InstanceWalkers.h"

#include "iscenegraph.h"

namespace scene
{

InstanceSubgraphWalker::InstanceSubgraphWalker(GraphPtr& sceneGraph) :
	_sceneGraph(sceneGraph)
{}

bool InstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	// greebo: Register this new node with the scenegraph
	if (!node->inScene())
	{
		_sceneGraph->insert(node);
		node->setSceneGraph(_sceneGraph);
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

UninstanceSubgraphWalker::UninstanceSubgraphWalker(Graph& sceneGraph) :
	_sceneGraph(sceneGraph)
{}

bool UninstanceSubgraphWalker::pre(const scene::INodePtr& node)
{
	return true;
}

void UninstanceSubgraphWalker::post(const scene::INodePtr& node)
{
	// Notify the Scenegraph about the upcoming deletion
	if (node->inScene())
	{
		_sceneGraph.erase(node);
		node->setSceneGraph(GraphPtr());
	}
}

} // namespace scene
