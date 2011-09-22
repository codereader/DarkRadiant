#pragma once

#include "inode.h"
#include <stack>

namespace scene
{

class Graph;
typedef boost::shared_ptr<Graph> GraphPtr;

/**
 * greebo: This Walker instantiates the visited nodes.
 * The whole subgraph is traversed and GlobalSceneGraph().insert() is
 * called on each node.
 */
class InstanceSubgraphWalker :
	public scene::NodeVisitor
{
private:
	std::stack<INodePtr> _nodeStack;

	GraphPtr& _sceneGraph;
public:
	InstanceSubgraphWalker(GraphPtr& sceneGraph);

	bool pre(const INodePtr& node);
	void post(const INodePtr& node);
};

/**
 * greebo: This Walker un-instantiates the visited nodes
 * The whole subgraph is traversed and erase() is
 * called on each nodes, AFTER it has been traversed.
 *
 * Contrary to InstanceSubgraphWalker this class is taking
 * a direct reference to Graph, since it is invoked in the
 * SceneGraph destructor.
 */
class UninstanceSubgraphWalker :
	public scene::NodeVisitor
{
private:
	Graph& _sceneGraph;

public:
	UninstanceSubgraphWalker(Graph& sceneGraph);

	bool pre(const scene::INodePtr& node);
	void post(const scene::INodePtr& node);
};

} // namespace scene
