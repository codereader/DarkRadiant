#ifndef REMOVEFROMLAYERWALKER_H_
#define REMOVEFROMLAYERWALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class RemoveFromLayerWalker :
	public Graph::Walker,
	public SelectionSystem::Visitor,
	public NodeVisitor
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
	void visit(const INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		if (Node_isEntity(node)) {
			// We have an entity, traverse all children too
			node->traverse(const_cast<RemoveFromLayerWalker&>(*this));
		}
	}

	// Scene::Graph::Walker
	bool pre(const Path& path, const INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		return true;
	}

	// scene::NodeVisitor
	bool pre(const INodePtr& node) {
		node->removeFromLayer(_layer);
		return true;
	}
};

} // namespace scene

#endif /* REMOVEFROMLAYERWALKER_H_ */
