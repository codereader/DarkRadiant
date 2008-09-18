#ifndef SET_LAYER_SELECTED_WALKER_H_
#define SET_LAYER_SELECTED_WALKER_H_

#include "ilayer.h"
#include "scenelib.h"

namespace scene {

class SetLayerSelectedWalker :
	public NodeVisitor
{
	int _layer;
	bool _selected;

public:
	SetLayerSelectedWalker(int layer, bool selected) :
		_layer(layer),
		_selected(selected)
	{}

	// scene::NodeVisitor
	bool pre(const scene::INodePtr& node) {
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && entity->getKeyValue("classname") == "worldspawn") {
			// Skip the worldspawn
			return true;
		}

		LayerList layers = node->getLayers();

		if (layers.find(_layer) != layers.end()) {
			Node_setSelected(node, _selected);
		}

		return true;
	}
};

} // namespace scene

#endif /* SET_LAYER_SELECTED_WALKER_H_ */
