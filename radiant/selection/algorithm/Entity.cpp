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

	// Entites are getting accumulated in this list and processed at destruction time
	mutable std::set<scene::INodePtr> _entities;
public:
	EntitySetClassnameSelected(const std::string& classname) :
		_classname(classname)
	{}

	~EntitySetClassnameSelected() {
		for (std::set<scene::INodePtr>::iterator i = _entities.begin();
			 i != _entities.end(); i++)
		{
			// "Rename" the entity, this deletes the old node and creates a new one
			scene::INodePtr newNode = changeEntityClassname(*i, _classname);

			// Select the new entity node
			Node_setSelected(newNode, true);
		}
	}

	virtual void visit(const scene::INodePtr& node) const {
		// Check if we have an entity
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && !_classname.empty() && Node_isSelected(node)) { 
			_entities.insert(node);
		}
	}
};

void setEntityClassname(const std::string& classname) {
	// greebo: instantiate a walker and traverse the current selection
	EntitySetClassnameSelected classnameSetter(classname);
	GlobalSelectionSystem().foreachSelected(classnameSetter);

	// The destructor of the classNameSetter will rename the entities
}

	} // namespace algorithm
} // namespace selection
