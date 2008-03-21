#ifndef UPDATENODEVISIBILITYWALKER_H_
#define UPDATENODEVISIBILITYWALKER_H_

#include "ilayer.h"
#include "iscenegraph.h"

namespace scene {

class UpdateNodeVisibilityWalker :
	public scene::Graph::Walker
{
public:
	bool pre(const Path& path, const INodePtr& node) const {
		GlobalLayerSystem().updateNodeVisibility(node);
		return true;
	}
};

} // namespace scene

#endif /* UPDATENODEVISIBILITYWALKER_H_ */
