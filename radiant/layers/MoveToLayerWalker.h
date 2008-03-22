#ifndef MOVETOLAYERWALKER_H_
#define MOVETOLAYERWALKER_H_

#include "ilayer.h"
#include "iselection.h"

namespace scene {

class MoveToLayerWalker :
	public SelectionSystem::Visitor
{
	int _layer;

public:
	MoveToLayerWalker(int layer) :
		_layer(layer)
	{}

	void visit(const scene::INodePtr& node) const {
		// Move the node to the given layer
		node->moveToLayer(_layer);
	}
};

} // namespace scene

#endif /* MOVETOLAYERWALKER_H_ */
