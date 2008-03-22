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

public:
	/**
	 * Pass the ID of the layer the items should be removed from.
	 */
	RemoveFromLayerWalker(int layer) :
		_layer(layer)
	{}

	// SelectionSystem::Visitor
	void visit(const scene::INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);
	}

	// Scene::Graph::Walker
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		return true;
	}
};

} // namespace scene

#endif /* REMOVEFROMLAYERWALKER_H_ */
