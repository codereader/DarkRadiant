#ifndef ITRANSFORMNODE_H_
#define ITRANSFORMNODE_H_

#include "inode.h"

class Matrix4;

/// \brief A transform node.
class TransformNode
{
public:
    virtual ~TransformNode() {}
	/// \brief Returns the transform which maps the node's local-space into the local-space of its parent node.
	virtual const Matrix4& localToParent() const  = 0;
};
typedef boost::shared_ptr<TransformNode> TransformNodePtr;

inline TransformNodePtr Node_getTransformNode(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<TransformNode>(node);
}

#endif /*ITRANSFORMNODE_H_*/
