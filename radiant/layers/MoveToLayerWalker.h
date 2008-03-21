#ifndef MOVETOLAYERWALKER_H_
#define MOVETOLAYERWALKER_H_

#include "ilayer.h"
#include "iselection.h"
#include "scenelib.h"

namespace scene {

class MoveToLayerWalker :
	public SelectionSystem::Visitor
{
	int _layer;

	mutable std::list<scene::INodePtr> _deselectList;
public:
	MoveToLayerWalker(int layer) :
		_layer(layer)
	{}

	// The destructor unselects all nodes that have been hidden during traversal.
	~MoveToLayerWalker() {
		for (std::list<scene::INodePtr>::iterator i = _deselectList.begin();
			 i != _deselectList.end(); i++)
		{
			Node_setSelected(*i, false);
		}
	}

	void visit(const scene::INodePtr& node) const {
		// Remove the layer from all previous layers
		LayerList layers = node->getLayers();

		for (LayerList::iterator i = layers.begin(); i != layers.end(); i++) {
			node->removeFromLayer(*i);
		}

		// Add the node to the given layer
		node->addToLayer(_layer);

		// Check the visibility of this node
		if (!GlobalLayerSystem().updateNodeVisibility(node)) {
			// node is hidden now, add to de-select list
			_deselectList.push_back(node);
		}
	}
};

} // namespace scene

#endif /* MOVETOLAYERWALKER_H_ */
