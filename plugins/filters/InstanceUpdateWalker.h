#ifndef INSTANCEUPDATEWALKER_H_
#define INSTANCEUPDATEWALKER_H_

#include "iscenegraph.h"
#include "ientity.h"
#include "ieclass.h"

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
			if (!GlobalFilterSystem().isVisible("entityclass", name)) {
				instance.setFiltered(true);
			}
			else {
				instance.setFiltered(false);
			}
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
