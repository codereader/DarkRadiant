#ifndef ADDTOLAYERWALKER_H_
#define ADDTOLAYERWALKER_H_

#include "ilayer.h"
#include "iselection.h"
#include "scenelib.h"

namespace scene {

class AddToLayerWalker :
	public SelectionSystem::Visitor
{
	int _layer;

	mutable std::list<scene::INodePtr> _deselectList;
public:
	AddToLayerWalker(int layer) :
		_layer(layer)
	{}

	// The destructor unselects all nodes that have been hidden during traversal.
	~AddToLayerWalker() {
		for (std::list<scene::INodePtr>::iterator i = _deselectList.begin();
			 i != _deselectList.end(); i++)
		{
			Node_setSelected(*i, false);
		}
	}

	void visit(const scene::INodePtr& node) const {
		node->addToLayer(_layer);

		if (!GlobalLayerSystem().updateNodeVisibility(node)) {
			// node is hidden now, add to de-select list
			_deselectList.push_back(node);
		}
	}
};

} // namespace scene

#endif /* ADDTOLAYERWALKER_H_ */
