#ifndef CLONEALLWALKER_H_
#define CLONEALLWALKER_H_

#include "inode.h"

namespace map
{

inline scene::CloneablePtr Node_getCloneable(const scene::INodePtr& node)
{
	return boost::dynamic_pointer_cast<scene::Cloneable>(node);
}

/**
 * greebo: Attempts to clone the given node. Returns the cloned node
 * on success or an empty INodePtr if the node is not cloneable.
 */
inline scene::INodePtr cloneSingleNode(const scene::INodePtr& node)
{
	scene::CloneablePtr cloneable = Node_getCloneable(node);

	// Return an empty node if not cloneable
	return (cloneable != NULL) ? cloneable->clone() : scene::INodePtr();
}

class CloneAll :
	public scene::NodeVisitor
{
	scene::Path _path;
public:
	CloneAll(const scene::INodePtr& root) :
		_path(root)
	{}

	bool pre(const scene::INodePtr& node)
	{
		if (node->isRoot()) {
			return false;
		}

		// Insert the cloned node or NULL if not cloneable
		_path.push(cloneSingleNode(node));

		return true;
	}

	void post(const scene::INodePtr& node)
	{
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
inline scene::INodePtr Node_Clone(const scene::INodePtr& node)
{
	scene::INodePtr clone = cloneSingleNode(node);

	CloneAll visitor(clone);
	node->traverseChildren(visitor);

	return clone;
}

} // namespace map

#endif /*CLONEALLWALKER_H_*/
