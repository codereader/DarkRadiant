#ifndef INSTANCEUPDATEWALKER_H_
#define INSTANCEUPDATEWALKER_H_

#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"
#include "ipatch.h"

#include "scenelib.h"

namespace filters {

class Deselector :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) {
		Node_setSelected(node, false);
		return true;
	}
};

/**
 * Scenegraph walker to update filtered status of Instances based on the
 * status of their parent entity class.
 */
class InstanceUpdateWalker : 
	public scene::Graph::Walker
{
public:
	// Pre-descent walker function
	bool pre(const scene::Path& path, const scene::INodePtr& node) const { 

		// Retrieve the parent entity and check its entity class.
		Entity* entity = Node_getEntity(node);
		if (entity != NULL) {
			IEntityClassConstPtr eclass = entity->getEntityClass();
			node->setFiltered(
				!GlobalFilterSystem().isVisible("entityclass", eclass->getName())
			);
		}
		
		// greebo: Update visibility of PatchInstances
		if (Node_isPatch(node)) {
			node->setFiltered(
				!GlobalFilterSystem().isVisible("object", "patch")
			);
		}
		
		// greebo: Update visibility of BrushInstances
		if (Node_isBrush(node)) {
			node->setFiltered(
				!GlobalFilterSystem().isVisible("object", "brush")
			);
		}

		if (!node->visible()) {
			// de-select this node and all children
			Deselector deselector;
			Node_traverseSubgraph(node, deselector);
		}

		// Continue the traversal
		return true;
	}
};

} // namespace filters

#endif /*INSTANCEUPDATEWALKER_H_*/
