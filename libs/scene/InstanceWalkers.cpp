#include "InstanceWalkers.h"

#include "iscenegraph.h"

namespace scene {

InstanceSubgraphWalker::InstanceSubgraphWalker(const scene::Path& path) :  
	_path(path)
{}

bool InstanceSubgraphWalker::pre(const scene::INodePtr& node) {
	_path.push(node);
	
	// Instantiate this node with the reference to the current parent instance
	node->instantiate(_path);

	// greebo: Register this new node with the scenegraph 
	GlobalSceneGraph().insert(node);

	return true;
}

void InstanceSubgraphWalker::post(const scene::INodePtr& node) {
	// The subgraph has been traversed, remove the top stack elements
	_path.pop();
}

// ==============================================================================================

UninstanceSubgraphWalker::UninstanceSubgraphWalker(const scene::Path& parent) : 
	_path(parent) // Initialise the path with the given parent path
{}
	
bool UninstanceSubgraphWalker::pre(const scene::INodePtr& node) {
	// Just remember what path we're going: push the node
	_path.push(node);
	return true;
}
	
void UninstanceSubgraphWalker::post(const scene::INodePtr& node) {
	// Instantiate this node with the reference to the current parent instance
	node->uninstantiate(_path);
	
	// Notify the Scenegraph about the upcoming deletion
	GlobalSceneGraph().erase(node);

	// We're done with this node, pop the path stack
	_path.pop();
}

} // namespace scene
