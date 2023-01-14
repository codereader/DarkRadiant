#pragma once

#include "ilayer.h"

namespace scene
{

class RemoveFromLayerWalker :
    public selection::SelectionSystem::Visitor,
    public NodeVisitor
{
private:
	int _layer;

public:
	/**
	 * Pass the ID of the layer the nodes should be removed from
	 */
	RemoveFromLayerWalker(int layer) :
		_layer(layer)
	{}

	// SelectionSystem::Visitor
	void visit(const INodePtr& node) const override
    {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		if (Node_isEntity(node))
        {
			// We have an entity, traverse all children too
			node->traverseChildren(const_cast<RemoveFromLayerWalker&>(*this));
		}
	}

	// scene::NodeVisitor
	bool pre(const INodePtr& node) override
    {
		node->removeFromLayer(_layer);
		return true;
	}
};

} // namespace
