#ifndef CLONEALLWALKER_H_
#define CLONEALLWALKER_H_

#include "inode.h"
#include "itraversable.h"

namespace map {

inline scene::CloneablePtr Node_getCloneable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<scene::Cloneable>(node);
}

/**
 * greebo: Attempts to clone the given node. Returns the cloned node
 * on success or an empty INodePtr if the node is not cloneable.
 */
inline scene::INodePtr cloneSingleNode(scene::INodePtr node) {
	scene::CloneablePtr cloneable = Node_getCloneable(node);
	if (cloneable != NULL) {
		return cloneable->clone();
	}
  
	// Return an empty node
	return scene::INodePtr();
}

class CloneAll : 
	public scene::NodeVisitor
{
	scene::Path _path;
public:
	CloneAll(scene::INodePtr root) : 
		_path(root)
	{}
	
	virtual bool pre(const scene::INodePtr& node) {
		if (node->isRoot()) {
			return false;
		}

		// Insert the cloned node or NULL if not cloneable
		_path.push(cloneSingleNode(node)); 

		return true;
	}

	virtual void post(const scene::INodePtr& node) {
		if (node->isRoot()) {
			return;
		}

		if (_path.top() != NULL) {
			// Cloning was successful, add to parent
			_path.parent()->addChildNode(_path.top());
		}

		_path.pop();
	}
};

/**
 * greebo: Attempts to clone the given node and all cloneable child nodes.
 *
 * @returns: the cloned node (which might own cloned children).
 */
inline scene::INodePtr Node_Clone(scene::INodePtr node) {
	scene::INodePtr clone = cloneSingleNode(node);

	CloneAll visitor(clone);
	node->traverse(visitor);
  
	return clone;
}

} // namespace map

#endif /*CLONEALLWALKER_H_*/
