#ifndef ASSIGN_LAYER_MAPPING_WALKER_H_
#define ASSIGN_LAYER_MAPPING_WALKER_H_

#include "inode.h"

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
		// Retrieve the next set of layer mappings
		const scene::LayerList& layers = _infoFile.getNextLayerMapping();

		if (layers.size() > 0) {
			scene::LayerList::const_iterator i = layers.begin();

			// Move the node to the first layer (so that it gets removed from all others)
			node->moveToLayer(*i);

			// Add the node to all remaining layers
			for (; i != layers.end(); i++) {
				node->addToLayer(*i);
			}
		}

		return true;
	}
};

} // namespace map

#endif /* ASSIGN_LAYER_MAPPING_WALKER_H_ */
