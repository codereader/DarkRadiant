#ifndef MOVETOLAYERWALKER_H_
#define MOVETOLAYERWALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class MoveToLayerWalker :
	public SelectionSystem::Visitor,
	public NodeVisitor
{
	int _layer;

public:
	MoveToLayerWalker(int layer) :
		_layer(layer)
	{}

	// SelectionSystem::Visitor
	void visit(const INodePtr& node) const {
		// Move the node to the given layer
		node->moveToLayer(_layer);

		if (Node_isEntity(node)) {
			// We have an entity, traverse all children too
			node->traverse(const_cast<MoveToLayerWalker&>(*this));
		}
	}

	// scene::NodeVisitor
	bool pre(const INodePtr& node) {
		node->moveToLayer(_layer);
		return true;
	}
};

} // namespace scene

#endif /* MOVETOLAYERWALKER_H_ */
