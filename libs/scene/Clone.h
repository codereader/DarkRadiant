#pragma once

#include "inode.h"
#include "iscenegraph.h"
#include <functional>

namespace scene
{

inline CloneablePtr Node_getCloneable(const INodePtr& node)
{
	return std::dynamic_pointer_cast<Cloneable>(node);
}

/**
 * greebo: Attempts to clone the given node. Returns the cloned node
 * on success or an empty INodePtr if the node is not cloneable.
 */
inline INodePtr cloneSingleNode(const INodePtr& node)
{
	CloneablePtr cloneable = Node_getCloneable(node);

	// Return an empty node if not cloneable
	return cloneable ? cloneable->clone() : INodePtr();
}

// Function that is invoked after a node has been cloned, called with <sourceNode, clonedNode>
typedef std::function<void(const INodePtr&, const INodePtr&)> PostCloneCallback;

class CloneAll :
	public NodeVisitor
{
private:
	Path _path;

	PostCloneCallback _postCloneCallback;

public:
	CloneAll(const INodePtr& root, const PostCloneCallback& callback) :
		_path(root),
		_postCloneCallback(callback)
	{}

	bool pre(const INodePtr& node) override
	{
		if (node->isRoot())
		{
			return false;
		}

		INodePtr cloned = cloneSingleNode(node);

		// Insert the cloned node or an empty ptr if not cloneable
		_path.push(cloned);

		return true;
	}

	void post(const INodePtr& node) override
	{
		if (node->isRoot())
		{
			return;
		}

		if (_path.top())
		{
			// Cloning was successful, add to parent
			_path.parent()->addChildNode(_path.top());

			if (_postCloneCallback)
			{
				_postCloneCallback(node, _path.top());
			}
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
inline INodePtr cloneNodeIncludingDescendants(const INodePtr& node, const PostCloneCallback& callback)
{
	INodePtr clone = cloneSingleNode(node);

	CloneAll visitor(clone, callback);
	node->traverseChildren(visitor);

	if (callback)
	{
		callback(node, clone);
	}

	return clone;
}

} // namespace
