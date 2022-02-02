#pragma once

#include "inode.h"

class Matrix4;

/// A node which can transform the coordinate space of its children
class ITransformNode
{
public:
    virtual ~ITransformNode() {}

	/// \brief Returns the transform which maps the node's local-space into the local-space of its parent node.
	virtual Matrix4 localToParent() const = 0;
};
typedef std::shared_ptr<ITransformNode> ITransformNodePtr;

/// An ITransformNode which can provide non-const access to its transform matrix
class IMatrixTransform: public ITransformNode
{
public:

    /// Set the value of the contained transformation matrix
	virtual void setLocalToParent(const Matrix4& localToParent) = 0;
};