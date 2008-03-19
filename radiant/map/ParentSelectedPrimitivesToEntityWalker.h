#ifndef PARENTSELECTEDTOENTITYWALKER_H_
#define PARENTSELECTEDTOENTITYWALKER_H_

#include "iscenegraph.h"

/**
 * Walker which traverses selected primitives and parents them to the given
 * entity.
 */
class ParentSelectedPrimitivesToEntityWalker : 
	public scene::Graph::Walker
{
	scene::INodePtr _newParent;
public:
	ParentSelectedPrimitivesToEntityWalker(const scene::INodePtr& newParent) : 
		_newParent(newParent)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (node != _newParent && Node_isPrimitive(node) && Node_isSelected(node)) {
			// We have a candidate, don't dig deeper
			return false;
		}
		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (node != _newParent && Node_isPrimitive(node) && Node_isSelected(node)) {
			// Get the parent of this node
			scene::INodePtr parent = path.parent();

			if (parent != _newParent) {
				// Copy the shared_ptr, the reference will be invalid once removed
				scene::INodePtr child(node);

				// Relocate this node from the old parent to the new one
				parent->removeChildNode(child);
				_newParent->addChildNode(child);
			}
		}
	}
};

#endif /* PARENTSELECTEDTOENTITYWALKER_H_ */
