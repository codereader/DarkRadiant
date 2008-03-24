#ifndef ASSIGN_LAYER_MAPPING_WALKER_H_
#define ASSIGN_LAYER_MAPPING_WALKER_H_

#include "scenelib.h"

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
		// Retrieve the next set of layer mappings and assign them
		assignNodeToLayers(node, _infoFile.getNextLayerMapping());
		return true;
	}
};

} // namespace map

#endif /* ASSIGN_LAYER_MAPPING_WALKER_H_ */
