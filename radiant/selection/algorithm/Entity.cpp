#include "Entity.h"

#include "iselection.h"
#include "entitylib.h"

#include "../../entity.h"

namespace selection {
	namespace algorithm {

/**
 * greebo: This walker traverses a subgraph and changes the classname
 *         of all selected entities to the one passed to the constructor.
 */
class EntitySetClassnameSelected : 
	public SelectionSystem::Visitor
{
	std::string _classname;
public:
	EntitySetClassnameSelected(const std::string& classname) :
		_classname(classname)
	{}

	virtual void visit(const scene::INodePtr& node) const {
		// Check if we have an entity
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && !_classname.empty() && Node_isSelected(node)) { 
			changeEntityClassname(node, _classname);
		}
	}
};

void setEntityClassname(const std::string& classname) {
	// greebo: instantiate a walker and traverse the current selection
	EntitySetClassnameSelected classnameSetter(classname);
	GlobalSelectionSystem().foreachSelected(classnameSetter);
}

	} // namespace algorithm
} // namespace selection
