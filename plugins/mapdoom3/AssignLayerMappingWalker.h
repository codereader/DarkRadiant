#ifndef ASSIGN_LAYER_MAPPING_WALKER_H_
#define ASSIGN_LAYER_MAPPING_WALKER_H_

#include "scenelib.h"
#include "imodel.h"

namespace map {

class AssignLayerMappingWalker :
	public scene::NodeVisitor
{
	InfoFile& _infoFile;
public:
	AssignLayerMappingWalker(InfoFile& infoFile) :
		_infoFile(infoFile)
	{}

	bool pre(const scene::INodePtr& node) {
		if (Node_isModel(node)) {
			// We have a model, assign the layers of the parent
			scene::INodePtr parent = node->getParent();
			if (parent != NULL) {
				assignNodeToLayers(node, parent->getLayers());
			}
			return true;
		}

		// Retrieve the next set of layer mappings and assign them
		assignNodeToLayers(node, _infoFile.getNextLayerMapping());
		return true;
	}
};

} // namespace map

#endif /* ASSIGN_LAYER_MAPPING_WALKER_H_ */
