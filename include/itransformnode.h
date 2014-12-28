#pragma once

#include "inode.h"

class Matrix4;

/// \brief A transform node.
class ITransformNode
{
public:
    virtual ~ITransformNode() {}

	/// \brief Returns the transform which maps the node's local-space into the local-space of its parent node.
	virtual const Matrix4& localToParent() const  = 0;
};
typedef std::shared_ptr<ITransformNode> ITransformNodePtr;

inline ITransformNodePtr Node_getTransformNode(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<ITransformNode>(node);
}
