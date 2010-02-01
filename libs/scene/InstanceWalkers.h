#ifndef SCENE_INSTANCE_WALKERS_H_
#define SCENE_INSTANCE_WALKERS_H_

#include "inode.h"
#include <stack>

namespace scene
{

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
public:
	bool pre(const INodePtr& node);
	void post(const INodePtr& node); 
};

/** 
 * greebo: This Walker un-instantiates the visited nodes 
 * The whole subgraph is traversed and erase() is 
 * called on each nodes, AFTER it has been traversed.
 */
class UninstanceSubgraphWalker : 
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node);
	void post(const scene::INodePtr& node);
};

} // namespace scene

#endif /* SCENE_INSTANCE_WALKERS_H_ */
