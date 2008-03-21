#ifndef ADDTOLAYERWALKER_H_
#define ADDTOLAYERWALKER_H_

#include "ilayer.h"
#include "iselection.h"

namespace scene {

class AddToLayerWalker :
	public SelectionSystem::Visitor
{
	int _layer;
public:
	AddToLayerWalker(int layer) :
		_layer(layer)
	{}

	void visit(const scene::INodePtr& node) const {
		node->addToLayer(_layer);

		GlobalLayerSystem().updateNodeVisibility(node);
	}
};

} // namespace scene

#endif /* ADDTOLAYERWALKER_H_ */
