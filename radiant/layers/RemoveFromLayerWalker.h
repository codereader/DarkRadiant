#ifndef REMOVEFROMLAYERWALKER_H_
#define REMOVEFROMLAYERWALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class RemoveFromLayerWalker :
	public scene::Graph::Walker,
	public SelectionSystem::Visitor
{
	int _layer;

	mutable std::list<scene::INodePtr> _deselectList;

	// If true, this walker updates the node visibility and 
	// de-selects hidden nodes after traversal
	bool _updateNodeVisibility;

public:
	/**
	 * Pass the ID of the layer the items should be removed from plus the boolean,
	 * which tells the walker whether to update the visibility status of the nodes 
	 * and de-select them after traversal or not.
	 */
	RemoveFromLayerWalker(int layer, bool updateNodeVisibility = true) :
		_layer(layer),
		_updateNodeVisibility(updateNodeVisibility)
	{}

	~RemoveFromLayerWalker() {
		for (std::list<scene::INodePtr>::iterator i = _deselectList.begin();
			 i != _deselectList.end(); i++)
		{
			Node_setSelected(*i, false);
		}
	}

	// SelectionSystem::Visitor
	void visit(const scene::INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		if (_updateNodeVisibility) {
			if (!GlobalLayerSystem().updateNodeVisibility(node)) {
				// node is hidden now, add to de-select list
				_deselectList.push_back(node);
			}
		}
	}

	// Scene::Graph::Walker
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		if (_updateNodeVisibility) {
			if (!GlobalLayerSystem().updateNodeVisibility(node)) {
				// node is hidden now, add to de-select list
				_deselectList.push_back(node);
			}
		}

		return false;
	}
};

} // namespace scene

#endif /* REMOVEFROMLAYERWALKER_H_ */
