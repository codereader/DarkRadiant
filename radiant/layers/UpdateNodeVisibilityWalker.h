#ifndef UPDATENODEVISIBILITYWALKER_H_
#define UPDATENODEVISIBILITYWALKER_H_

#include "ilayer.h"
#include "iscenegraph.h"
#include "scenelib.h"

namespace scene {

class UpdateNodeVisibilityWalker :
	public scene::Graph::Walker
{
public:
	bool pre(const Path& path, const INodePtr& node) const {
		// Update the visibility and check if the node is visible now
		if (!GlobalLayerSystem().updateNodeVisibility(node)) {
			// Node is hidden after update, de-select
			Node_setSelected(node, false);
		}
		return true;
	}
};

} // namespace scene

#endif /* UPDATENODEVISIBILITYWALKER_H_ */
