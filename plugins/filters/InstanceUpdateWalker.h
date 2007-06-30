#ifndef INSTANCEUPDATEWALKER_H_
#define INSTANCEUPDATEWALKER_H_

#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"
#include "ipatch.h"

#include "scenelib.h"

namespace filters
{

/**
 * Scenegraph walker to update filtered status of Instances based on the
 * status of their parent entity class.
 */
class InstanceUpdateWalker
: public scene::Graph::Walker
{
public:

	// Pre-descent walker function
	bool pre(const scene::Path& path, scene::Instance& instance) const { 

		// Retrieve the parent entity and check its entity class.
		Entity* entity = Node_getEntity(path.top());
		if (entity) {
			IEntityClassConstPtr eclass = entity->getEntityClass();
			std::string name = eclass->getName();
			instance.setFiltered(
				!GlobalFilterSystem().isVisible("entityclass", name)
			);
		}
		
		// greebo: Update visibility of PatchInstances
		if (Node_isPatch(path.top())) {
			instance.setFiltered(
				!GlobalFilterSystem().isVisible("object", "patch")
			);
		}
		
		// greebo: Update visibility of BrushInstances
		if (Node_isBrush(path.top())) {
			instance.setFiltered(
				!GlobalFilterSystem().isVisible("object", "brush")
			);
		}
		
		// Continue the traversal
		return true;
	}

	// Post descent function
	void post(const scene::Path& path, scene::Instance& instance) const {
		
	}
};

}

#endif /*INSTANCEUPDATEWALKER_H_*/
