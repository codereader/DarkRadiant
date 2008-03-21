#ifndef REMOVEFROMLAYERWALKER_H_
#define REMOVEFROMLAYERWALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class RemoveFromLayerWalker :
	public scene::Graph::Walker
{
	int _layer;

public:
	RemoveFromLayerWalker(int layer) :
		_layer(layer)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		// Remove the node from this layer
		node->removeFromLayer(_layer);

		// greebo: Note that any node should be de-selected, when
		// it becomes hidden. This should be taken care of by the
		// LayerSystem after traversal of this walker.

		return false;
	}
};

} // namespace scene

#endif /* REMOVEFROMLAYERWALKER_H_ */
