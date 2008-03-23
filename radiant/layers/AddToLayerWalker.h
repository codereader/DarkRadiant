#ifndef ADDTOLAYERWALKER_H_
#define ADDTOLAYERWALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class AddToLayerWalker :
	public SelectionSystem::Visitor,
	public NodeVisitor
{
	int _layer;

public:
	AddToLayerWalker(int layer) :
		_layer(layer)
	{}

	void visit(const scene::INodePtr& node) const {
		node->addToLayer(_layer);

		if (Node_isEntity(node)) {
			// We have an entity, traverse all children too
			node->traverse(const_cast<AddToLayerWalker&>(*this));
		}
	}

	// scene::NodeVisitor
	bool pre(const scene::INodePtr& node) {
		node->addToLayer(_layer);
		return true;
	}
};

} // namespace scene

#endif /* ADDTOLAYERWALKER_H_ */
