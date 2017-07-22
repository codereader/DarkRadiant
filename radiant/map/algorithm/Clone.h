#pragma once

#include "inode.h"
#include "iscenegraph.h"
#include <functional>

namespace map
{

inline scene::CloneablePtr Node_getCloneable(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<scene::Cloneable>(node);
}

/**
 * greebo: Attempts to clone the given node. Returns the cloned node
 * on success or an empty INodePtr if the node is not cloneable.
 */
inline scene::INodePtr cloneSingleNode(const scene::INodePtr& node)
{
	scene::CloneablePtr cloneable = Node_getCloneable(node);

	// Return an empty node if not cloneable
	return cloneable ? cloneable->clone() : scene::INodePtr();
}

// Function that is invoked after a node has been cloned, called with <sourceNode, clonedNode>
typedef std::function<void(const scene::INodePtr&, const scene::INodePtr&)> PostCloneCallback;

class CloneAll :
	public scene::NodeVisitor
{
private:
	scene::Path _path;

	PostCloneCallback _postCloneCallback;

public:
	CloneAll(const scene::INodePtr& root, const PostCloneCallback& callback) :
		_path(root),
		_postCloneCallback(callback)
	{}

	bool pre(const scene::INodePtr& node) override
	{
		if (node->isRoot())
		{
			return false;
		}

		scene::INodePtr cloned = cloneSingleNode(node);

		if (cloned && _postCloneCallback)
		{
			_postCloneCallback(node, cloned);
		}

		// Insert the cloned node or an empty ptr if not cloneable
		_path.push(cloned);

		return true;
	}

	void post(const scene::INodePtr& node) override
	{
		if (node->isRoot())
		{
			return;
		}

		if (_path.top())
		{
			// Cloning was successful, add to parent
			_path.parent()->addChildNode(_path.top());
		}

		_path.pop();
	}
};

/**
 * greebo: Attempts to clone the given node and all cloneable child nodes.
 * The PostCloneCallback will be invoked for every node that is successfully cloned,
 * to give the calling code an opportunity for post-processing.
 * 
 * @returns: the cloned node (which might own cloned children).
 */
inline scene::INodePtr cloneNodeIncludingDescendants(const scene::INodePtr& node, 
	const PostCloneCallback& callback)
{
	scene::INodePtr clone = cloneSingleNode(node);

	if (callback)
	{
		callback(node, clone);
	}

	CloneAll visitor(clone, callback);
	node->traverseChildren(visitor);

	// Cloned child nodes are assigned the layers of the source nodes
	// update the layer visibility flags to make the layers assignemnt take effect
	scene::UpdateNodeVisibilityWalker visibilityUpdater;
	clone->traverse(visibilityUpdater);

	return clone;
}

} // namespace map
