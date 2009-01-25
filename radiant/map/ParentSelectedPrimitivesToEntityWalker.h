#ifndef PARENTSELECTEDTOENTITYWALKER_H_
#define PARENTSELECTEDTOENTITYWALKER_H_

#include "iselection.h"

/**
 * Walker which traverses selected primitives and parents them to the given
 * entity.
 */
class ParentSelectedPrimitivesToEntityWalker : 
	public SelectionSystem::Visitor
{
	scene::INodePtr _newParent;
public:
	ParentSelectedPrimitivesToEntityWalker(const scene::INodePtr& newParent) : 
		_newParent(newParent)
	{}

	void visit(const scene::INodePtr& node) const {
		if (node != _newParent && Node_isPrimitive(node)) {
			// We have a candidate, make a copy just for sure
			scene::INodePtr child(node);

			child->getParent()->removeChildNode(child);
			_newParent->addChildNode(child);
		}
	}
};

#endif /* PARENTSELECTEDTOENTITYWALKER_H_ */
